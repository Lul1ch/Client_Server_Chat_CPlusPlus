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

#include <pthread.h>
#include <semaphore.h>

using namespace std;

#define PORT 1504
#define BUFSIZE 1024

int networkSocket = -1;
bool isExit = false;

void* ClientThread(void* args)
{  
   string buffer;
   string username = ((char*)args); 
   
   networkSocket = socket(AF_INET, SOCK_STREAM, 0);

   struct sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = INADDR_ANY;
   serverAddress.sin_port = htons(PORT);

   int connectionStatus = connect(networkSocket, (struct sockaddr*)&serverAddress,
                                 sizeof(serverAddress));

   if (connectionStatus < 0) 
   {
      puts("Error\n");
      return 0;
   }
   
   while (!isExit)
   {
  
      getline(cin,  buffer);
      if (buffer.compare("/exit") == 0)
      {       
         buffer = "User has exit!";
         isExit = true;
      }

      string message;
      message.clear();
      message = username + ": " + buffer;    
      send(networkSocket, message.c_str(), message.length(),0);

   }
     
   close(networkSocket);
   pthread_exit(NULL);
   
  
}

void* MessagesListener(void* args) {
   char buffer[BUFSIZE];
   while(!isExit) {
      if(networkSocket != -1) {
         int lastIndex = recv(networkSocket, (char*)buffer,BUFSIZE,0);
         buffer[lastIndex] = '\0';
         cout <<  buffer << "\n";
      }

   }
   cout << "The end of the reader work!\n";
   pthread_exit(NULL);
}

int main (int argc, char* argv[])
{  
   pthread_t tid;
   pthread_create(&tid, NULL, ClientThread, argv[1]);
   
   pthread_t reciver;
   pthread_create(&reciver, NULL, MessagesListener, (void*)0);
   
   pthread_join(reciver, NULL);
   pthread_join(tid, NULL);
   return 0;
}
