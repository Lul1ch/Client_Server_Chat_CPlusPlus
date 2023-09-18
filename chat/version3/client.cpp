#include <sys/types.h>
#include <iostream>
#include <netinet/in.h>
//#include <bits/stdc++.h>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <map>
#include <thread>
#include <chrono>
using namespace std;

#define PORT 1504
#define BUFSIZE 1024

bool isExit = false;
string inputStr;
bool isMessage;
bool isCommand;
int networkSocket = -1;

class ConnectionsManager
{
   
   void MessagesListener() 
   {
      char buffer[BUFSIZE];
      while(!isExit) {
         if(networkSocket != -1) {
            int lastIndex = recv(networkSocket, (char*)buffer,BUFSIZE,0);
            buffer[lastIndex] = '\0';
            cout <<  buffer << "\n";
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

      while (!isExit)
      {
         if(isMessage && !isExit)
         {
            send(networkSocket, inputStr.c_str(), inputStr.length(),0);
            isMessage = false;
         } else {
            this_thread::sleep_for(chrono::milliseconds(2000));
         }
      }
     
      close(networkSocket);
      pthread_exit(NULL);  
   }
public:
   ConnectionsManager()
   {
      ConnectionsManager tempObj;
      thread reciver(&ConnectionsManager::MessagesListener, &tempObj);
      thread client(&ConnectionsManager::ClientThread, &tempObj);
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
  /* map<string, Command*> funcMap = {
      {"/exit", new Exit}
   };*/
   void Exit() {
      isExit = true;
   }
   void ExecuteCommand(string commandName) {
      if (commandName == "/exit") {
         Exit();     
      }
   }
   void CommandsHandlerRoutine()
   {
      
      while(!isExit)
      {
         if(isCommand && !isExit)
         {
            string commandName = inputStr.substr(0, inputStr.find(" "));
           //funcMap[commandName]->Execute();           
            ExecuteCommand(commandName);
            isCommand = false;
         } else {
            this_thread::sleep_for(chrono::milliseconds(2000));
         }
      }
   }

public:
   CommandsHandler()
   {
      CommandsHandler tempObj;
      thread handler(&CommandsHandler::CommandsHandlerRoutine, &tempObj);
   }   
};

class InputHandler
{
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
            isCommand = true;
         } else 
         { 
            isMessage = true;
         }
      }
   }
public:
   InputHandler()
   {
      InputListener();            
   }
   
};

int main (int argc, char* argv[])
{
   ConnectionsManager* conManager = new ConnectionsManager();
   CommandsHandler* commsHandler = new CommandsHandler(); 
   InputHandler* input = new InputHandler();
   
   delete conManager;
   delete commsHandler;
   delete input;
   return 0;
}
