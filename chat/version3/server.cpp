#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>

#include <map>
#include <string>
#include <netinet/in.h>

#include <thread>
#include <utility>
#include <mutex>
#include "output.h"
#include "messagesBroker.h"

using namespace std;

#define PORT 1504
#define BUFSIZE 1024
#define MAXCLIENTS 50

bool isStop = false;
OutputHandler outputHandler;

class ConnectionManager;

template<>
void MessagesBroker<ConnectionManager>::TerminateTheSubscriberThread(int key);

template<>
void MessagesBroker<ConnectionManager>::TerminateAllSubscribersThreads();

class ClientsCommandsHandler
{
public:
   void AnalyzeCommand(const Message* command,  MessagesBroker<ConnectionManager>* msgsBroker, int clientSocket)
   {  
      string commandStr = command->GetMessage();
      string commandName = commandStr.substr(0, commandStr.find(" "));  
      if(commandName == "/exit")
      {  
         msgsBroker->IsMessageToSend(new Message(command->GetAuthorSocket(), string("User " + to_string(command->GetAuthorSocket()) + " logged out!")));
         msgsBroker->TerminateTheSubscriberThread(clientSocket);
         msgsBroker->EraseSubscriber(clientSocket);

      } else {
     
      }
   }
};

class ServerCommandsHandler
{
public:
   void AnalyzeCommand(const string command, MessagesBroker<ConnectionManager>* msgsBroker)
   {   
      string commandName = command.substr(0, command.find(" "));  
      if(commandName == "/stop")
      {       
         msgsBroker->IsMessageToSend(new Message(-1, string("The server is stopped!")));//TODO Заменить магическое число -1
         msgsBroker->TerminateAllSubscribersThreads();
         isStop = true;
      } else {
   
      }
   }
};

class IncomingMessagesHandler
{
   ClientsCommandsHandler handler;

   bool IsItCommand(string commandName)
   {
      return commandName[0] == '/'; 
   }

public:
   void ReciveIncomingMessage(const Message* message, MessagesBroker<ConnectionManager>* msgsBroker, const int clientSocket)
   {
      if (IsItCommand(message->GetMessage()))
      {
         handler.AnalyzeCommand(message, msgsBroker, clientSocket);           
      } else {
         outputHandler.PrintClientMessage(message->GetMessage(), message->GetAuthorSocket());
         msgsBroker->IsMessageToSend(message);
      }        
   }
};

class ConnectionManager
{
   int clientSocket;
   IncomingMessagesHandler incMsgsHandler;
   MessagesBroker<ConnectionManager>* msgsBroker;
   bool isExit = false;
   thread manager;
   
   void Client()
   {
      char buffer[BUFSIZE];

      while(!isExit) 
      {
         int lastIndex = recv(clientSocket, (char *)buffer, BUFSIZE, 0);
         buffer[lastIndex] = '\0';
         if (!isExit)
         {
            incMsgsHandler.ReciveIncomingMessage(new Message(clientSocket, string(buffer)), msgsBroker, clientSocket);          
         }
      }

      outputHandler.PrintClientMessage(string("User " + to_string(clientSocket) + " logged out!"), clientSocket);        
      close(clientSocket);
   }

public:   
   ConnectionManager(const int socketNum, MessagesBroker<ConnectionManager>* msgsBroker)
   {
      this->clientSocket = socketNum;
      this->msgsBroker = msgsBroker;
      manager = thread(&ConnectionManager::Client, this);
      manager.detach();
   }

   void SendMessage(const Message& msg)
   {
      if (clientSocket != msg.GetAuthorSocket())
      {
         send(clientSocket, msg.GetMessage().c_str(), msg.GetMessage().size(), 0);
      }
   }

   void IsExit()
   {
      isExit = true;
   }
};


template<>
void MessagesBroker<ConnectionManager>::TerminateTheSubscriberThread(int key)
{
   subscribersMap[key]->IsExit();
}

template<>
void MessagesBroker<ConnectionManager>::TerminateAllSubscribersThreads()
{
      for(auto& element: subscribersMap)
      {
         element.second->IsExit();
      }
}

class ClientsConnectionsHandler 
{

   int  server, newSocket;  

   struct sockaddr_in serverAddr;
   struct sockaddr_storage serverStorage;
   
   MessagesBroker<ConnectionManager>* msgsBroker;
   thread handler;

   int InitServer()
   { 
      server = socket(AF_INET, SOCK_STREAM, 0);

      if (server < 0)
      {
         cout << "\nError establishing socket...\n";
         exit(EXIT_FAILURE);
      }

      serverAddr.sin_family = AF_INET;
      serverAddr.sin_addr.s_addr = INADDR_ANY;
      serverAddr.sin_port = htons(PORT);

      const int optval = 1;
      setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
      
      if ((bind(server, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) < 0)
      {
         cout << "Error binding connection, the socket has already been established...\n";  
         isStop = true;
         return -1;
      }
      if (listen(server, MAXCLIENTS) == 0)
         cout << "Listening\n";
      else
         cout << "Error\n";
   }
  
   void HandlerRoutine() 
   {
      
      while (!isStop)
      {
         socklen_t addr_size = sizeof(serverStorage);
         //TODO: Решить как завершить прослушку подключений клиента при остановке сервера
         newSocket = accept(server, (struct sockaddr*)&serverStorage, &addr_size);   
         if (!isStop) 
         {
            ConnectionManager* manager = new ConnectionManager(newSocket, msgsBroker);
            msgsBroker->AddSubscriber(newSocket, manager);                
         }
      }
   }
public:
   ClientsConnectionsHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      InitServer();
      this->msgsBroker = msgsBroker;
      handler = thread(&ClientsConnectionsHandler::HandlerRoutine, this);
      //handler.detach();
   }

   void JoinRoutineThread()
   {
      handler.join();
   }

   ~ClientsConnectionsHandler()
   {
      close(server);
   }
};


class InputHandler
{
   MessagesBroker<ConnectionManager>* msgsBroker;
   bool IsItCommand(string str)
   {
      return str[0] == '/';
   }   

   void Input() 
   {
      string input;
      while(!isStop) 
      {
         getline(cin, input);
         if (IsItCommand(input))
         {
            ServerCommandsHandler handler;
            handler.AnalyzeCommand(input, msgsBroker);
         } else {
            outputHandler.PrintServerMessage("Unknown input!\n");   
         }
      }
   }

public:
   InputHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      this->msgsBroker = msgsBroker;
      Input();
   }
};

int main ()
{
   MessagesBroker<ConnectionManager>* msgsBroker = new MessagesBroker<ConnectionManager>(&isStop);
   ClientsConnectionsHandler* clientsConnectsHandler = new ClientsConnectionsHandler(msgsBroker);
   InputHandler* inputHandler = new InputHandler(msgsBroker);

   msgsBroker->JoinRoutineThread();
   clientsConnectsHandler->JoinRoutineThread();
   
   delete msgsBroker;
   delete clientsConnectsHandler;
   delete inputHandler;
   return 0;
}  
