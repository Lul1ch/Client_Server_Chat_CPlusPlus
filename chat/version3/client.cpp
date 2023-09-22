#include <sys/types.h>
#include <iostream>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <map>
#include <thread>
#include <chrono>
#include <mutex>
using namespace std;

#define PORT 1504
#define BUFSIZE 1024

bool isExit = false;

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

class ConnectionsManager
{
   bool isMessage;
   mutex isMessageMutex;
   int networkSocket;
   string inputStr;
   
   thread reciver;
   thread client;  
   
   void MessagesListener() 
   {
      char buffer[BUFSIZE];
      while(!isExit) {
         if(networkSocket != -1) {
            int lastIndex = recv(networkSocket, (char*)buffer,BUFSIZE,0);
            buffer[lastIndex] = '\0';
            cout << buffer << "\n";
         }

      }
   }
   
   int EstablishTheConnection()
   {

      networkSocket = socket(AF_INET, SOCK_STREAM, 0);

      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_addr.s_addr = INADDR_ANY;
      serverAddress.sin_port = htons(PORT); //TODO: Узнать, что делает эта функция

      int connectionStatus = connect(networkSocket, (struct sockaddr*)&serverAddress,
                                 sizeof(serverAddress));
      return connectionStatus;
   }

   void ClientThread()
   {  
      if (EstablishTheConnection() < 0) //TODO: Узнать по каким причинам могло не установиться соединение
      {
         cout << "An error occurred during the attempt to establish the connection\n";
         isExit = true;
      }    

      while (!isExit)
      {
         if(isMessage && !isExit)
         {
            lock_guard<mutex> guard(isMessageMutex);
            cout << "Hello from messages thread " << networkSocket << "\n"; //TODO: Подумать над реализацией вывода дебага при передаче определённого флага запуска, например
            send(networkSocket, inputStr.c_str(), inputStr.length(),0);
            isMessage = false;
         } else if (!isExit){
            this_thread::sleep_for(chrono::milliseconds(1000));
         }
      }
      
      if (isMessage) 
      {
         send(networkSocket, inputStr.c_str(), inputStr.length(),0);
      }
      close(networkSocket);  
   }
public:
   ConnectionsManager()
   {
      this->isMessage = false;
      this->networkSocket = -1;

      reciver = thread(&ConnectionsManager::MessagesListener, this);
      client = thread(&ConnectionsManager::ClientThread, this);
      
      reciver.detach();
      client.detach();
   }

   void IsMessageToSend(string inputStr)
   {
      lock_guard<mutex> guard(isMessageMutex);
      isMessage = true;
      this->inputStr = inputStr;
      cout << "Message executed\n";
   }
};

/*class Command 
{
public:
   virtual void Execute()
   {

   }
   
   virtual string GetName()
   {

   }
};

class Exit : Command {
public:
   void Execute() override 
   {
      isExit = true;
   }

   string GetName() override
   {
      return "/exit";
   }
}*/
class CommandsHandler
{ 
   ConnectionsManager* manager;
   bool isCommand;
   mutex isCommandMutex;
   string inputStr;
 
   thread handler;  
   /* map<string, Command*> funcMap = {
      {"/exit", new Exit}
   };TODO вынести название команды в наследников Command, автоматическая инициализация каждой команды */
   void Exit() 
   {
      manager->IsMessageToSend(inputStr);
      isExit = true;
   }
   void ExecuteCommand(string commandName) 
   {
      if(commandName == "/exit")
      {
         Exit();
      } else {
         cout << "\nUnknown command\n"; 
      }
   }
   void CommandsHandlerRoutine()
   {
      while(!isExit)
      {      
         if(isCommand)
         {
            cout << "Hello from command thread\n";
            lock_guard<mutex> guard(isCommandMutex);
            string commandName = inputStr.substr(0, inputStr.find(" "));
            cout << "|" << commandName << "|\n"; 
           //funcMap[commandName]->Execute();
            ExecuteCommand(commandName);
            isCommand = false;
         } else {
            this_thread::sleep_for(chrono::milliseconds(1000));
         }
      }
   }
//TODO Попробовать разыменовать NULL ptr
public:
   CommandsHandler(ConnectionsManager* manager)
   {
      this->manager = manager;
      this->isCommand = false;
      handler = thread(&CommandsHandler::CommandsHandlerRoutine, this);
      handler.detach();
   }
   void IsCommandToHandle(string inputStr)
   {
      lock_guard<mutex> guard(isCommandMutex);
      isCommand = true;
      this->inputStr = inputStr;
      cout << "Command executed\n";
   }   
};

class InputHandler
{
   ConnectionsManager* manager;
   CommandsHandler* handler;
   string inputStr;
   
   bool IsItCommand(string str)
   {
      return str[0] == '/';
   }  

public:
   InputHandler(ConnectionsManager* manager, CommandsHandler* handler)
   {
      this->manager = manager;
      this->handler = handler;           
   }
   
   void InputListener()
   {
      while (!isExit) 
      {
         getline(cin, inputStr);
         if (IsItCommand(inputStr))
         {
            cout << "Command\n";
            handler->IsCommandToHandle(inputStr);
         } else 
         {
            cout << "Message\n";
            manager->IsMessageToSend(inputStr);
         }
         
      }
   }
};
//TODO структурировать в голове знания по поводу указателя и ссылки
int main (int argc, char* argv[])
{
   ConnectionsManager* conManager = new ConnectionsManager();
   CommandsHandler* commandsHandler = new CommandsHandler(conManager);
   InputHandler* input = new InputHandler(conManager, commandsHandler);
   input->InputListener();

   delete conManager;
   delete commandsHandler;
   delete input;
   return 0;
}
