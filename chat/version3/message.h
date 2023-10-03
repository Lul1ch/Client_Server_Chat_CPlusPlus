#include <string>

class Message
{
   int authorSocket;
   std::string message;
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
      this->authorSocket = -1;
      this->message = "Hi!";
   }

   Message(int authorSocket, std::string message)
   {
      this->authorSocket = authorSocket;
      this->message = message;
   }

   Message& operator=(const Message& msg)
   { 
      this->authorSocket = msg.GetAuthorSocket();
      this->message = msg.GetMessage();
      return *this;
   }
};
