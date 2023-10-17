#pragma once
#include <iostream>
#include <string>
#include <atomic>

#define NO_AUTHOR -1
class Message final
{
   int authorSocket;
   std::string message;
   static std::atomic<int> messagesCounter;
public:
   std::string GetMessage() const
   {
      return message;
   }

   int GetAuthorSocket() const
   {
      return authorSocket;
   }
   
   Message()
   {
      //std::cout << "Message is created1\n";      
      this->authorSocket = -1;
      this->message = "Hi!";
   }

   Message(int authorSocket, std::string message)
   {
      messagesCounter++;
      //std::cout << "Counter increased " << messagesCounter << "\n";      
      this->authorSocket = authorSocket;
      this->message = message;
   }

   Message& operator=(const Message& msg)
   { 
      this->authorSocket = msg.GetAuthorSocket();
      this->message = msg.GetMessage();
      return *this;
   }

   ~Message()
   {
      messagesCounter--;
      //std::cout << "Counter decreased " << messagesCounter << "\n";      
   }
};

std::atomic<int> Message::messagesCounter{0};
