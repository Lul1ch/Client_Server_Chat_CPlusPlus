#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <vector>
#include <string>
#include <netinet/in.h>

using namespace std;

#define PORT 1504
#define BUFSIZE 1024
#define MAXCLIENTS 50

sem_t output;
pthread_t clientsThreads[MAXCLIENTS*2];
int clientsCount = 0;
bool isStop = false;
vector<int> clientsSockets;
int curClientsNum = 0;
 
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

   Message(int authorSocket, string message)
   {
      this->authorSocket = authorSocket;
      this->message = message;
   }
};

class ConnectionManager 
{

   void SendMessageToOtherClients(int clientSocket, char* message)
   {
      for (int i = 0; i < curClientsNum; i++) 
      {
         if (clientsSockets[i] != clientSocket) 
         {
            send(clientsSockets[i], (const char *)message, strlen(message), 0);
         }
      }     
   }

   int FindClientIndex(int socket)
   {
      for (int i = curClientsNum - 1; i >= 0; i--) {
         if (clientsSockets[i] == socket) {
            return i;
         }
      }
   }

   void Client(int socket)
   {
      int newSocket = socket;
      bool isExit = false;
      char buffer[BUFSIZE];
      int indexInVec = FindClientIndex(newSocket);
      while(!isExit) {
         int lastIndex = recv(newSocket, (char *)buffer, BUFSIZE, 0);
         buffer[lastIndex] = '\0';
 

         SendMessageToOtherClients(newSocket, buffer);
      
      }
      pthread_exit(NULL);
   }
   void ManagerRoutine()
   {
   
   }
public:
   ConnectionManager(int socketNum)
   {
      ConnectionManager tempObj;     
      thread manager(&tempObj::Client, &tempObj, socketNum);
   }
};
class ClientsConnectionsHandler 
{

   int  server, newSocket;  

   struct sockaddr_in serverAddr;
   struct sockaddr_storage serverStorage;
   
   void HandlerRoutine() 
   {
      
      while (!isStop)
      {
         addr_size = sizeof(serverStorage);
   
         newSocket = accept(server, (struct sockaddr*)&serverStorage, &addr_size);   
      
         clientsSockets.push_back(newSocket);
         curClientsNum++;
     
         pthread_create(&clientsThreads[i++], NULL, Client, &newSocket);
         if (i >= MAXCLIENTS) {
            i = 0;
            while (i < MAXCLIENTS) {
               pthread_join(clientsThreads[i++], NULL);
            }
            i = 0;
         }
      }
      close(server);
   }
public:
   ClientsConnectionsHandler()
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
      //Vstavit' conditions variable     
      ClientsConnectionsHandler tempObj;
      thread handler(&tempObj::HandlerRoutine, &tempObj);
   }
};

class IncomigMessagesHandler
{
   bool IsItCommand(string commandName)
   {
      return commandName[0] == '/'; 
   }

public:
   void ReciveIncomingMessage(Message message)
   {
      if (IsItCommand(message.GetMessage()))
      {
      
      } else {
         sem_wait(&output);     
         cout << message.GetMessage() << "\n";
         sem_post(&output);
         //запрос на рассылку
      }        
   }
};

class MessagesBroker
{

public:
   SendMessage(Message msg)
   {

   }
};

class CommandsHandler
{
   void AnalyzeCommand(Message commandName)
   {
      switch(commandName)
      {
         case "/exit":
   
            break;
         default:
            //Запрос на рассылку сообщения о том, что команда не распознана
      }  
   }
   
   void HandlerRoutine()
   {
      while(!isStop) 
      {
         if (true)
         { 
            AnalyzeCommand();
            string message = string(buffer);
            if (message.find("User has exit!") != -1)
            {
               isExit = true;
               clientsSockets.erase(clientsSockets.begin() + indexInVec);
               curClientsNum--;
            }
         }    
      }   
   }
public:
   CommandsHandler()
   {
      CommandsHandler tempObj;
      thread handler(&tempObj::HandlerRoutine, &tempObj);
   }
};

class InputHandler
{

   void Input() 
   {
      string input;
      while(!isStop) 
      {
         getline(cin, input);
         if (input.compare("/stop") == 0)
         {
            sem_wait(&output);
            cout << "Server has stopped\n";
            sem_post(&output);
            isStop = true;
         }
      }
   }

public:
   InputHandler()
   {
      Input();
   }
};
int main ()
{
   
   socklen_t addr_size;
   sem_init(&output, 0, 1);
   
   return 0;
}  
