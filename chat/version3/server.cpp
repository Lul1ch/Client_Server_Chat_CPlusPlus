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
//TODO сделать удаление ссылки на ConnectionManager из вектора в Шине Сообщений при окончании его основного потока
//TODO Перенести инициализацию переменных классов на одну табуляцию правее
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
   
   void CopyData(Message* msg)
   {
      this->authorSocket = msg->GetAuthorSocket();
      this->message = msg->GetMessage();
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
IncomingMessagesHandler incMsgsHandler;
MessagesBroker* msgsBroker;
bool isExit = false;
thread manager;
   
   void Client(int socket);
public:
   ConnectionManager(int socketNum, MessagesBroker* msgsBroker);

   void SendMessage(Message msg);

   void IsExit();   
};
class MessagesBroker {
thread broker;
vector<ConnectionManager*> connManagersVec;
Message message;
bool isMessage;

   void BrokerRoutine();
public:
   void IsMessageToSend(Message*);

   MessagesBroker();

   void AddManager(ConnectionManager* manager);
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
         msgsBroker->IsMessageToSend(new Message(command->GetAuthorSocket(), string("User " + to_string(command->GetAuthorSocket()) + " logged out!")));
         manager->IsExit();
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
}
   
ConnectionManager::ConnectionManager(int socketNum, MessagesBroker* msgsBroker)
{
   this->clientSocket = socketNum;
   this->msgsBroker = msgsBroker;
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
   
         newSocket = accept(server, (struct sockaddr*)&serverStorage, &addr_size);   
      
         ConnectionManager* manager = new ConnectionManager(newSocket, msgsBroker);
         msgsBroker->AddManager(manager);                
      }
      close(server);
   }
public:
   ClientsConnectionsHandler(MessagesBroker* msgsBroker)
   {
      InitServer();
      this->msgsBroker = msgsBroker;
      //TODO Vstavit' conditions variable     
      handler = thread(&ClientsConnectionsHandler::HandlerRoutine, this);
      handler.detach();
   }
};

void MessagesBroker::BrokerRoutine()
{
   while(!isStop)
   {
      if (isMessage)
      {
         //TODO Вставить lock_guard
         for(int i = 0; i < connManagersVec.size(); i++)
         {
            connManagersVec[i]->SendMessage(message);
         }
         isMessage = false;
      }   
   }
}

void MessagesBroker::IsMessageToSend(Message* msg)
{
   //TODO вставить lock_guard
   isMessage = true;
   this->message = *msg;
   //this->message.CopyData(msg);
}

MessagesBroker::MessagesBroker()
{
   broker = thread(&MessagesBroker::BrokerRoutine, this);  
   broker.detach();
}

void MessagesBroker::AddManager(ConnectionManager* manager)
{
   connManagersVec.push_back(manager);
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
            outputHandler.PrintMessage("Unknown input!\n", 0);//TODO Убрать вывод или заменить магическое число 0        
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
   
   delete msgsBroker;
   delete clientsConnectsHandler;
   delete inputHandler;
   
   return 0;
}  
