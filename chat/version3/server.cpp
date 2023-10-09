#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

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
#define MAXCLIENTS 10

bool isStop = false;
OutputHandler outputHandler;

class ConnectionManager;

template<>
void MessagesBroker<ConnectionManager>::TerminateTheSubscriberThread(int key);

template<>
void MessagesBroker<ConnectionManager>::TerminateAllSubscribersThreads();

class ExecutableClient
{
   class CommandClient
   {
   public:
      virtual void Execute(const Message* message){}

      virtual string GetName(){ return "/command"; }
   };

   class Exit : public  CommandClient
   {
      MessagesBroker<ConnectionManager>* msgsBroker;
   public:
      void Execute(const Message* message) override 
      {
         int clientSocket = message->GetAuthorSocket();
         msgsBroker->IsMessageToSend(new Message(clientSocket, string("User " + to_string(clientSocket) + " logged out!")));
         msgsBroker->TerminateTheSubscriberThread(clientSocket);
         msgsBroker->EraseSubscriber(clientSocket);
      }

      string GetName() override
      {
         return "/exit";
      }

      Exit(MessagesBroker<ConnectionManager>* msgsBroker)
      {
         this->msgsBroker = msgsBroker;
      }
   };

   map<string, CommandClient*> funcMapClient;
public:

   ExecutableClient(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      funcMapClient.insert(make_pair("/exit", new Exit(msgsBroker)));
   }
   
   void ExecuteCommandClient(const Message* command)
   {
      string commandName = command->GetMessage().substr(0, command->GetMessage().find(" "));
      funcMapClient[commandName]->Execute(command);
   }
};

class ClientsCommandsHandler
{
ExecutableClient* clientCommands;
public:
   ClientsCommandsHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      clientCommands = new ExecutableClient(msgsBroker);
   }
   void AnalyzeCommand(const Message* command)
   {  
      clientCommands->ExecuteCommandClient(command);
   }
};

class ExecutableServer
{
   //Стоит ли вынести в общий класс/заголовочный файл
   class CommandServ
   {
   public:
      virtual void Execute(const string inputStr){}

      virtual string GetName(){ return "/command"; }
   };

   class Stop : public  CommandServ 
   {
   public:
      void Execute(const string inputStr) override 
      {
         isStop = true;
      }

      string GetName() override
      {
         return "/exit";
      }
   };

   map<string, CommandServ*> funcMapServ;
public:

   ExecutableServer()
   {
      funcMapServ.insert(make_pair("/stop", new Stop()));
   }
   
   void ExecuteCommand(string inputStr)
   {
      string commandName = inputStr.substr(0, inputStr.find(" "));
      funcMapServ[commandName]->Execute(inputStr);
   }
};

class ServerCommandsHandler
{
ExecutableServer* serverCommands = new ExecutableServer();
public:
   void AnalyzeCommand(const string command)
   {   
      serverCommands->ExecuteCommand(command);
   }
};

class IncomingMessagesHandler
{
   ClientsCommandsHandler* handler;

   bool IsItCommand(string commandName)
   {
      return commandName[0] == '/'; 
   }

public:
   IncomingMessagesHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      handler = new ClientsCommandsHandler(msgsBroker);
   }
   void ReciveIncomingMessage(const Message* message, MessagesBroker<ConnectionManager>* msgsBroker)
   {
      if (IsItCommand(message->GetMessage()))
      {
         handler->AnalyzeCommand(message);           
      } else {
         outputHandler.PrintClientMessage(message->GetMessage(), message->GetAuthorSocket());
         msgsBroker->IsMessageToSend(message);
      }        
   }
};

class ConnectionManager
{
   int clientSocket;
   MessagesBroker<ConnectionManager>* msgsBroker; 
   IncomingMessagesHandler* incMsgsHandler;
   bool isExit = false;
   thread manager;
   
   void Client()
   {
      char buffer[BUFSIZE];

      while(!isExit) 
      {    
         int lastIndex = recv(clientSocket, (char*)buffer, BUFSIZE, 0);
         if (lastIndex != -1)
         {
            buffer[lastIndex] = '\0';
            incMsgsHandler->ReciveIncomingMessage(new Message(clientSocket, string(buffer)), msgsBroker);          
         }
         this_thread::sleep_for(chrono::milliseconds(10));
      }
      outputHandler.PrintClientMessage(string("User " + to_string(clientSocket) + " logged out!"), clientSocket);        
      close(clientSocket);
   }

public:   
   ConnectionManager(const int socketNum, MessagesBroker<ConnectionManager>* msgsBroker)
   {
      this->clientSocket = socketNum;
      this->msgsBroker = msgsBroker;

      incMsgsHandler = new IncomingMessagesHandler(msgsBroker);
      
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

   void SendMessageOnlyToThisClient(const Message& msg)
   {
      send(clientSocket, msg.GetMessage().c_str(), msg.GetMessage().size(), 0);
   }
   
   void ChangeExitFlag(bool value)
   {
      isExit = value;
   }
};


template<>
void MessagesBroker<ConnectionManager>::TerminateTheSubscriberThread(int key)
{
   subscribersMap[key]->ChangeExitFlag(true);
}

template<>
void MessagesBroker<ConnectionManager>::TerminateAllSubscribersThreads()
{
      for(auto& element: subscribersMap)
      {
         element.second->SendMessage(*(new Message(-1, "!stop"))); //TODO Заменить магическое число -1
         element.second->ChangeExitFlag(true);
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
      server = socket(AF_INET, SOCK_STREAM, 0); //SOCK_STREAM

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
      
      struct timeval rcvTime;
      rcvTime.tv_sec = 0;
      rcvTime.tv_usec = 10000;
      setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, &rcvTime, sizeof(rcvTime));
     
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
         if (newSocket != -1) 
         {
            ConnectionManager* manager = new ConnectionManager(newSocket, msgsBroker);
            msgsBroker->AddSubscriber(newSocket, manager);                
         }
         this_thread::sleep_for(chrono::milliseconds(10));
      }
   }
public:
   ClientsConnectionsHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      InitServer();
      this->msgsBroker = msgsBroker;
      handler = thread(&ClientsConnectionsHandler::HandlerRoutine, this);
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

   ServerCommandsHandler handler;
   MessagesBroker<ConnectionManager>* msgsBroker;
   
   bool IsItCommand(string str)
   {
      return str[0] == '/';
   }   

public:
   InputHandler(MessagesBroker<ConnectionManager>* msgsBroker)
   {
      this->msgsBroker = msgsBroker;
   }

   void ListenInput() 
   {
      string input;
      while(!isStop) 
      {
         getline(cin, input);
         if (IsItCommand(input))
         {
            handler.AnalyzeCommand(input);
         } else {
            outputHandler.PrintServerMessage("Unknown input!");   
         }
      }
   }
};

int main ()
{
   MessagesBroker<ConnectionManager>* msgsBroker = new MessagesBroker<ConnectionManager>(&isStop);
   ClientsConnectionsHandler* clientsConnectsHandler = new ClientsConnectionsHandler(msgsBroker);
   InputHandler* inputHandler = new InputHandler(msgsBroker);
   
   inputHandler->ListenInput();

   msgsBroker->JoinRoutineThread();
   clientsConnectsHandler->JoinRoutineThread();
   
   delete msgsBroker;
   delete clientsConnectsHandler;
   delete inputHandler;
   return 0;
}  
