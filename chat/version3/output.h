#include <iostream>
#include <mutex>
#include <string>

class OutputHandler
{
std::mutex output;
bool isDebug = false;
public:
   void PrintClientMessage(std::string str, int socketNum)
   {
      std::lock_guard<std::mutex> guard(output);
      std::cout << socketNum << ": " << str << "\n";
   }

   void PrintServerMessage(std::string str)
   {
      std::lock_guard<std::mutex> guard(output);
      std::cout << "Server: " << str << "\n";
   }

   void PrintRecivedMessage(std::string str)
   {
      std::lock_guard<std::mutex> guard(output);
      std::cout << str << "\n";
   }

   void DebugLog( std::string str)
   {
      if(isDebug)
      { 
         std::lock_guard<std::mutex> guard(output);
         std::cout << "Debug: " << str << "\n";
      }
   }

   void IsDebuging()
   {
      isDebug = true;
   }
};

