#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>

#include <vector>
#include <string>
#include <netinet/in.h>

#include <thread>
#include <mutex>
#include "output.h"
using namespace std;

#define PORT 1504
#define BUFSIZE 1024
#define MAXCLIENTS 50

bool isStop = false;
OutputHandler outputHandler;

template <class T>
class smart_lock_guard : lock_guard<T> {
bool value;
bool& varToChange;
public: 
   smart_lock_guard(bool& varToChange, bool value, T& mutex) : lock_guard<T>(mutex), varToChange(varToChange) 
   {
      this->value = value;
   }
   
   ~smart_lock_guard()
   {
      varToChange = value;
   }
};

class Message
{
   int authorSocket;
   string message;
public:
   string GetMessage()
   {
      return message;
   }

   int GetAuthorSocket()
   {
      return authorSocket;
   }
   
   Message()
   {
      this->authorSocket = -1;
      this->message = "Hi!";
   }

   Message(int authorSocket, string message)
   {
      this->authorSocket = authorSocket;
      this->message = message;
   }

   Message& operator=(Message& msg)
   { 
      this->authorSocket = msg.GetAuthorSocket();
      this->message = msg.GetMessage();
      return *this;
   }
};

class MessagesBroker;
class ConnectionManager;

class IncomingMessagesHandler
{
   bool IsItCommand(string commandName);
public:
   void ReciveIncomingMessage(Message* message, MessagesBroker* msgsBroker, ConnectionManager* manager);
};

class ConnectionManager{
   int clientSocket;
   int indexInVec;
   IncomingMessagesHandler incMsgsHandler;
   MessagesBroker* msgsBroker;
   bool isExit = false;
   thread manager;
   
   void Client(int socket);
public:
   ConnectionManager(int socketNum, MessagesBroker* msgsBroker, int indexInVec);

   void SendMessage(Message msg);

   void IsExit();   

   int GetIndexInVec();
};
class MessagesBroker {
   thread broker;
   vector<ConnectionManager*> connManagersVec;
   Message message;
   bool isMessage;
   mutex messageMutex;

   void BrokerRoutine();
public:
   void IsMessageToSend(Message*);

   MessagesBroker();

   void AddManager(ConnectionManager* manager);

   void EraseManager(int index);

   int GetVectorSize();

   void JoinRoutineThread();
};

class ClientsCommandsHandler
{
public:
   void AnalyzeCommand(Message* command,  MessagesBroker* msgsBroker, ConnectionManager* manager)
   {  
      string commandStr = command->GetMessage();
      string commandName = commandStr.substr(0, commandStr.find(" "));  
      if(commandName == "/exit")
      {
         manager->IsExit();
         
         msgsBroker->IsMessageToSend(new Message(command->GetAuthorSocket(), string("User " + to_string(command->GetAuthorSocket()) + " logged out!")));
         msgsBroker->EraseManager(manager->GetIndexInVec());

         outputHandler.PrintMessage(string("User " + to_string(command->GetAuthorSocket()) + " logged out!"), command->GetAuthorSocket());        
      } else {
     
      }
   }
};

class ServerCommandsHandler
{
public:
   void AnalyzeCommand(string command, MessagesBroker* msgsBroker)
   {   
      string commandName = command.substr(0, command.find(" "));  
      if(commandName == "/stop")
      {
         //TODO дописать логику, при которой будут останавливаться все текущие клиентские потоки, и будет отправляться сообщение об этом клиентам.         
         isStop = true;
      } else {
   
      }
   }
};
 
   bool IncomingMessagesHandler::IsItCommand(string commandName)
   {
      return commandName[0] == '/'; 
   }

   void IncomingMessagesHandler::ReciveIncomingMessage(Message* message, MessagesBroker* msgsBroker, ConnectionManager* manager)
   {
      if (IsItCommand(message->GetMessage()))
      {
         ClientsCommandsHandler handler;
         handler.AnalyzeCommand(message, msgsBroker, manager);           
      } else {
         outputHandler.PrintMessage(message->GetMessage(), message->GetAuthorSocket());
         msgsBroker->IsMessageToSend(message);
      }        
   }


void ConnectionManager::Client(int socket)
{
   clientSocket = socket;
   char buffer[BUFSIZE];

   while(!isExit) {
      int lastIndex = recv(clientSocket, (char *)buffer, BUFSIZE, 0);
      buffer[lastIndex] = '\0';
      incMsgsHandler.ReciveIncomingMessage(new Message(clientSocket, string(buffer)), msgsBroker, this);          
   }
   close(socket);
}
   
ConnectionManager::ConnectionManager(int socketNum, MessagesBroker* msgsBroker, int indexInVec)
{
   this->clientSocket = socketNum;
   this->msgsBroker = msgsBroker;
   this->indexInVec = indexInVec;
   manager = thread(&ConnectionManager::Client, this, socketNum);
   manager.detach();
}

void ConnectionManager::SendMessage(Message msg)
{
   if (clientSocket != msg.GetAuthorSocket())
   {
      send(clientSocket, msg.GetMessage().c_str(), msg.GetMessage().size(), 0);
   }
}

void ConnectionManager::IsExit()
{
   isExit = true;
}

int ConnectionManager::GetIndexInVec()
{
   return indexInVec;
}
class ClientsConnectionsHandler 
{

   int  server, newSocket;  

   struct sockaddr_in serverAddr;
   struct sockaddr_storage serverStorage;
   
   MessagesBroker* msgsBroker;
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
            ConnectionManager* manager = new ConnectionManager(newSocket, msgsBroker, msgsBroker->GetVectorSize());
            msgsBroker->AddManager(manager);                
         }
      }
   }
public:
   ClientsConnectionsHandler(MessagesBroker* msgsBroker)
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

void MessagesBroker::BrokerRoutine()
{
   while(!isStop)
   {
      if (isMessage)
      {
         smart_lock_guard<mutex> guard(isMessage, false, messageMutex);
         for(int i = 0; i < connManagersVec.size(); i++)
         {
            connManagersVec[i]->SendMessage(message);
         }
      }   
   }
}

void MessagesBroker::IsMessageToSend(Message* msg)
{
   smart_lock_guard<mutex> guard(isMessage, true, messageMutex);  
   this->message = *msg;
}

MessagesBroker::MessagesBroker()
{
   broker = thread(&MessagesBroker::BrokerRoutine, this);  
}

void MessagesBroker::AddManager(ConnectionManager* manager)
{
   connManagersVec.push_back(manager);
}

void MessagesBroker::EraseManager(int indexInVec)
{
   delete connManagersVec[indexInVec];
   connManagersVec.erase(connManagersVec.begin() + indexInVec);
}

int MessagesBroker::GetVectorSize()
{
   return connManagersVec.size();
}

void MessagesBroker::JoinRoutineThread()
{
   broker.join();
}
class InputHandler
{
   MessagesBroker* msgsBroker;
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
   InputHandler(MessagesBroker* msgsBroker)
   {
      this->msgsBroker = msgsBroker;
      Input();
   }
};

int main ()
{
   MessagesBroker* msgsBroker = new MessagesBroker();
   ClientsConnectionsHandler* clientsConnectsHandler = new ClientsConnectionsHandler(msgsBroker);
   InputHandler* inputHandler = new InputHandler(msgsBroker);

   msgsBroker->JoinRoutineThread();
   clientsConnectsHandler->JoinRoutineThread();
   
   delete msgsBroker;
   delete clientsConnectsHandler;
   delete inputHandler;
   return 0;
}  
