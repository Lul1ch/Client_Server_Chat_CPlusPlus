#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <string>
#include <string.h>

using namespace std;

#define PORT 1504
#define BUFSIZE 1024


int main (int argc, char* argv[])
{
   string username = argv[1]; 
   if (username.length() == 0) {
      cout << "Can't pass an unlogged user";
   }
   int client;
   bool isExit = false;
   string ip = "127.0.0.1";
   string buffer;
   struct sockaddr_in server_addr;

   client = socket(AF_INET, SOCK_DGRAM, 0);

   if (client < 0)
   {
      cout << "\nError establishing socket...\n";
      exit(EXIT_FAILURE);
   }
   
   memset(&server_addr, 0, sizeof(server_addr));

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(PORT);
   server_addr.sin_addr.s_addr = INADDR_ANY;

   socklen_t length;

   cout << "\n Enter /exit to end the connection\n";

   do {

      getline(cin,  buffer);
      if (buffer.compare("/exit") == 0)
      {
         //cout << "The end\n";         
         buffer = "User has exit!";
         isExit = true;
      }
      string wrapper;
      wrapper.clear();
      wrapper = username + ": " + buffer;    
      const char* message = wrapper.c_str();
      sendto(client, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&server_addr,
            sizeof(server_addr));
   } while (!isExit);
   
   cout << "The end of the session\n";   
   close(client);

   return 0;
}
