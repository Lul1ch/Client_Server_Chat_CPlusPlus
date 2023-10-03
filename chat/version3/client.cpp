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
#include "smartLockGuard.h"
#include "output.h"
using namespace std;

#define PORT 1504
#define BUFSIZE 1024

bool isExit = false;
bool isDebug = false;
OutputHandler outputHandler;

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
            outputHandler.PrintRecivedMessage(string(buffer));
         }

      }
   }
   
   int EstablishTheConnection()
   {

      networkSocket = socket(AF_INET, SOCK_STREAM, 0);

      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_addr.s_addr = INADDR_ANY;
      serverAddress.sin_port = htons(PORT); //host-to-network short меняет в памяти местами байты, располагая их в прямой последовательности.

      int connectionStatus = connect(networkSocket, (struct sockaddr*)&serverAddress,
                                 sizeof(serverAddress));
      return connectionStatus;
   }

   void ClientThread()
   {  
      if (EstablishTheConnection() < 0) //Причины найдены, но как их обрабатывать пока не понятно.
      {
         cout << "An error occurred during the attempt to establish the connection\n";
         isExit = true;
      }    

      while (!isExit)
      {
         if(isMessage && !isExit)
         {
            smart_lock_guard<mutex> guard(isMessage, false, isMessageMutex);
            outputHandler.DebugLog("Hello from messages thread\n");
            send(networkSocket, inputStr.c_str(), inputStr.length(),0);
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
      smart_lock_guard<mutex> guard(isMessage, true, isMessageMutex);
      this->inputStr = inputStr;
      
      outputHandler.DebugLog("Message sent\n");
   }
};

/*class Command 
{
public:
   virtual void Execute(string inputStr);
   
   virtual string GetName();
};

class Exit : Command {
public:
   void Execute(string inputStr) override 
   {
      manager->IsMessageToSend(inputStr);
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
            outputHandler.DebugLog("Hello from command thread\n");
            smart_lock_guard<mutex> guard(isCommand, false, isCommandMutex);
            string commandName = inputStr.substr(0, inputStr.find(" "));
           //funcMap[commandName]->Execute();
            ExecuteCommand(commandName);
         } else {
            this_thread::sleep_for(chrono::milliseconds(1000));
         }
      }
   }
//Попробовать разыменовать nullptr -> затирает весь вывод в консоль после вывода разыменованного указателя
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
      smart_lock_guard<mutex> guard(isCommand, true, isCommandMutex);
      this->inputStr = inputStr;
      outputHandler.DebugLog("Command executed\n");
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
            outputHandler.DebugLog("Command\n");
            handler->IsCommandToHandle(inputStr);
         } else 
         {
            outputHandler.DebugLog("Message\n");
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
