#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string>
using namespace std;

#define PORT 1504
#define BUFSIZE 1024

int main ()
{
   int  server;
   bool isExit = false;
   char buffer[BUFSIZE];

   struct sockaddr_in server_addr, cli_addr;
   socklen_t size;

   server = socket(AF_INET, SOCK_DGRAM, 0);

   if (server < 0)
   {
      cout << "\nError establishing socket...\n";
      exit(EXIT_FAILURE);
   }

   memset(&server_addr, 0, sizeof(server_addr));
   memset(&cli_addr, 0, sizeof(cli_addr));

   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = INADDR_ANY;
   server_addr.sin_port = htons(PORT);

   if ((bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
   {
      cout << "Error binding connection, the socket has already been established...\n";  
      return -1;
   }

   size = sizeof(cli_addr);

   while (!isExit)
   {
      int lastIndex = recvfrom(server, (char *)buffer, BUFSIZE, MSG_WAITALL, (struct sockaddr *)&cli_addr, &size);
      buffer[lastIndex] = '\0';
      cout << buffer << "\n";
      string message = string(buffer);
      if (message.find("User has exit!") != -1)
      {
         isExit = true;
      }    
      
   }
   close(server);
   return 0;
}
