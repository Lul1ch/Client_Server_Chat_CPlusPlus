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
            cout << "\n" <<  buffer << "\n";
         }

      }
      pthread_exit(NULL);
   }
   
   int EstablishTheConnection()
   {

      networkSocket = socket(AF_INET, SOCK_STREAM, 0);

      struct sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_addr.s_addr = INADDR_ANY;
      serverAddress.sin_port = htons(PORT);

      int connectionStatus = connect(networkSocket, (struct sockaddr*)&serverAddress,
                                 sizeof(serverAddress));
      return connectionStatus;
   }

   void ClientThread()
   {  
      if (EstablishTheConnection() < 0)
      {
         cout << "An error occurred during the attempt to establish the connection\n";
         isExit = true;
      }    
      isMessage = false;
      while (!isExit)
      {
         if(isMessage)
         {
            lock_guard<mutex> guard(isMessageMutex);
            cout << "Hello from messages thread " << networkSocket << "\n";
            send(networkSocket, inputStr.c_str(), inputStr.length(),0);
            isMessage = false;
            //isMessageMutex.unlock();
         } else {
            this_thread::sleep_for(chrono::milliseconds(1000));
         }
      }
     
      close(networkSocket);  
   }
public:
   ConnectionsManager()
   {

   }
   ConnectionsManager(int i)
   {
      this->isMessage = false;
      this->networkSocket = -1;

      ConnectionsManager tempObj;
      reciver = thread(&ConnectionsManager::MessagesListener, &tempObj);
      client = thread(&ConnectionsManager::ClientThread, &tempObj);
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
};

class Exit : Command {
public:
   void Execute() override 
   {
      isExit = true;
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
   };*/
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
      isCommand = false;
      while(!isExit)
      {      
         if(isCommand)
         {
            cout << "Hello from command thread\n";
            lock_guard<mutex> guard(isCommandMutex);
            string commandName = inputStr.substr(0, inputStr.find(" "));
           //funcMap[commandName]->Execute();
            ExecuteCommand(commandName);
            isCommand = false;
            //isCommandMutex.unlock();
         } else {
            this_thread::sleep_for(chrono::milliseconds(1000));
         }
      }
   }

public:
   CommandsHandler()
   {

   }
   CommandsHandler(ConnectionsManager* manager)
   {
      this->manager = manager;
      this->isCommand = false;
      CommandsHandler tempObj;
      handler = thread(&CommandsHandler::CommandsHandlerRoutine, &tempObj);
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
public:
   InputHandler(ConnectionsManager* manager, CommandsHandler* handler)
   {
      this->manager = manager;
      this->handler = handler;
      InputListener();            
   }
   
};

int main (int argc, char* argv[])
{
   ConnectionsManager* conManager = new ConnectionsManager(1);
   CommandsHandler* commandsHandler = new CommandsHandler(conManager);
   InputHandler* input = new InputHandler(conManager, commandsHandler);

   delete conManager;
   delete commandsHandler;
   delete input;
   return 0;
}
