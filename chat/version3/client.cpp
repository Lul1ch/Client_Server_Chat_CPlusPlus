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
#include <memory>
#include "smartLockGuard.h"
#include "output.h"
using namespace std;

#define PORT 1504
#define BUFSIZE 1024

bool isExit = false;
bool isDebug = false;
OutputHandler outputHandler;

class Executable
{

   class Command 
   {
   public:
      virtual void Execute(string inputStr){}

      virtual string GetName(){ return "/command"; }
   };

   class Exit : public  Command 
   {
   public:
      void Execute(string inputStr) override 
      {
         isExit = true;
      }

      string GetName() override
      {
         return "/exit";
      }
   };

   map<string, Command*> funcMap;
public:

   Executable()
   {
      funcMap.insert(make_pair("/exit", new Exit()));
   }
   
   void ExecuteCommand(string inputStr)
   {
      string commandName = inputStr.substr(0, inputStr.find(" "));
      if (funcMap.count(commandName) > 0)
      {
         funcMap[commandName]->Execute(inputStr);
      } else {
         outputHandler.PrintMessage("Unknown command!");
      }
   }
};

class CommandsHandler
{ 
   bool isCommand;
   bool isCommandResult;
   mutex isCommandMutex;
   string inputStr;
 
   thread handler;  
   Executable* commands = new Executable();

   void CommandsHandlerRoutine()
   {
      while(!isExit)
      {      
         outputHandler.DebugLog("Hello from command thread");
         if(isCommand)
         {
            smart_lock_guard<mutex> guard(isCommand, false, isCommandMutex);
            commands->ExecuteCommand(inputStr);
         } else if (isCommandResult) {
            smart_lock_guard<mutex> guard(isCommandResult, false, isCommandMutex);  
           //Вставить код обработки результатов работы команд, когда такие появятся
         } else {
            this_thread::sleep_for(chrono::milliseconds(10));
         }
      }
   }
//Попробовать разыменовать nullptr -> затирает весь вывод в консоль после вывода разыменованного указателя
public:
   CommandsHandler()
   {
      this->isCommand = false;
      handler = thread(&CommandsHandler::CommandsHandlerRoutine, this);
      handler.detach();
   }

   void IsCommandToHandle(string inputStr)
   {
      smart_lock_guard<mutex> guard(isCommand, true, isCommandMutex);
      this->inputStr = inputStr;
      outputHandler.DebugLog("Command executed");
   }   

   void IsCommandResultToHandle(string inputStr)
   {
      smart_lock_guard<mutex> guard(isCommandResult, true, isCommandMutex);
      this->inputStr = inputStr;
      outputHandler.DebugLog("Command result recived");
   }

   bool IsItStopCommandResult(string inputStr)
   {
      if (inputStr.find("!stop") >= 0)
      {
         outputHandler.PrintMessage("The server is stopped. Press Enter to finish the programm...");
         isExit = true;
      }
      return isExit;
   }
};

class RecivedMessagesHandler
{
   bool IsItCommandResult(string str)
   {
      return str[0] == '!';
   }
public:
   void AnalyzeRecivedMessage(string str, shared_ptr<CommandsHandler> commandsHandler)
   {
      if (IsItCommandResult(str))
      {
         if (!commandsHandler->IsItStopCommandResult(str))
         {
            commandsHandler->IsCommandResultToHandle(str);
         } 
      } else {
         outputHandler.PrintMessage(str);
      }
   }
};

class ConnectionsManager
{
   bool isMessage;
   mutex isMessageMutex;
   int networkSocket;
   queue<string> msgsQueue;
   
   thread reciver;
   thread client;  
   
   RecivedMessagesHandler rcvMsgsHandler;
   shared_ptr<CommandsHandler> commandsHandler;
   void MessagesListener() 
   {
      char buffer[BUFSIZE];
      int lastIndex = 0;
      while(!isExit && lastIndex != -1) {
         if(networkSocket > 0) {
            lastIndex = recv(networkSocket, (char*)buffer,BUFSIZE,0);
            buffer[lastIndex] = '\0';
            rcvMsgsHandler.AnalyzeRecivedMessage(string(buffer), commandsHandler);        
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
         if(isMessage)
         {
            smart_lock_guard<mutex> guard(isMessage, false, isMessageMutex);
            outputHandler.DebugLog("Hello from messages thread");
            
            int queueSize = msgsQueue.size();
            for (int i = 0; i < queueSize;i++)
            {
               string curMsg = msgsQueue.front();
               send(networkSocket, curMsg.c_str(), curMsg.length(),0);  
               msgsQueue.pop();  
            }
         } else {
            this_thread::sleep_for(chrono::milliseconds(10));
         }
      }
      
      string exitNotification = "/exit";
      send(networkSocket, exitNotification.c_str(), exitNotification.length(),0);

      close(networkSocket);  
   }
public:
   ConnectionsManager(shared_ptr<CommandsHandler> commandsHandler)
   {
      this->isMessage = false;
      this->networkSocket = -1;
      this->commandsHandler = commandsHandler;
      reciver = thread(&ConnectionsManager::MessagesListener, this);
      client = thread(&ConnectionsManager::ClientThread, this);
      
      reciver.detach();
      client.detach();
   }

   void IsMessageToSend(string inputStr)
   {
      smart_lock_guard<mutex> guard(isMessage, true, isMessageMutex);
      this->msgsQueue.push(inputStr);
      
      outputHandler.DebugLog("Message sent");
   }
};

class InputHandler
{
   shared_ptr<ConnectionsManager> manager;
   shared_ptr<CommandsHandler> handler;
   string inputStr;
   bool IsItCommand(string str)
   {
      return str[0] == '/';
   }  

public:
   InputHandler(shared_ptr<ConnectionsManager> manager, shared_ptr<CommandsHandler> handler)
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
            outputHandler.DebugLog("Command");
            handler->IsCommandToHandle(inputStr);
         } else 
         {
            outputHandler.DebugLog("Message");
            manager->IsMessageToSend(inputStr);
         }
         
      }
   }
};

class Spamer
{
   shared_ptr<ConnectionsManager> manager;
   thread spamer;
   string inputStr = "DSADjhgkfdlspweoritkg,vc..,mvbgkrdls;sdlfkghmbv,cdldrkg,vcd;pdrkg,vc;dporfvikirfk";                       
   void SpamerRoutine()
   {
      while(!isExit)
      {
         manager->IsMessageToSend(inputStr);
         this_thread::sleep_for(chrono::milliseconds(1));
      }
   }
public:
   Spamer(shared_ptr<ConnectionsManager> manager)
   {
      this->manager = manager;
      for (int i = 0; i < 0; i++)
      {
         inputStr += inputStr;
      }
      spamer = thread(&Spamer::SpamerRoutine, this);
   }
};

int main (int argc, char* argv[])
{
   shared_ptr<CommandsHandler> commandsHandler(new CommandsHandler());
   shared_ptr<ConnectionsManager> conManager(new ConnectionsManager(commandsHandler));
   unique_ptr<InputHandler> input(new InputHandler(conManager, commandsHandler));
   unique_ptr<Spamer> spamer(new Spamer(conManager));
   input->InputListener();
   
   return 0;
}
