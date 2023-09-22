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
void* Client(void* args)
{
   int newSocket = *((int*)args);
   bool isExit = false;
   char buffer[BUFSIZE];
   int indexInVec = FindClientIndex(newSocket);
   while(!isExit) {
      int lastIndex = recv(newSocket, (char *)buffer, BUFSIZE, 0);
      buffer[lastIndex] = '\0';
 
      sem_wait(&output);     
      cout << newSocket << ": " <<buffer << "\n";
      sem_post(&output);

      SendMessageToOtherClients(newSocket, buffer);
      
      string message = string(buffer);
      if (message.find("/exit") != -1)
      {
         isExit = true;
         clientsSockets.erase(clientsSockets.begin() + indexInVec);
         curClientsNum--;
         
         sem_wait(&output);     
         cout << "Client number " << newSocket << " has exited\n";
         sem_post(&output);
      }    
   }
   pthread_exit(NULL);
}

void* Input(void* args) 
{
   string input;
   bool isExit;
   while(!isExit) 
   {
      getline(cin, input);
      if (input.compare("/stop") == 0)
      {
         sem_wait(&output);
         cout << "Server has stopped\n";
         sem_post(&output);
         isStop = true;
         isExit = true;
      }
   }
   pthread_exit(NULL);
}

int main ()
{
   int  server, newSocket;  

   struct sockaddr_in serverAddr;
   struct sockaddr_storage serverStorage;
   
   socklen_t addr_size;
   sem_init(&output, 0, 1);
   
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
  
   int i = 0; 
   pthread_t inputListener;
   pthread_create(&inputListener, NULL, Input, (void*)0);  
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
   return 0;
}  
