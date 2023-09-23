#include <iostream>
#include <mutex>
#include <string>

class OutputHandler
{
std::mutex output;
public:
   void PrintMessage(std::string str, int socketNum)
   {
      std::lock_guard<std::mutex> guard(output);
      std::cout << socketNum << ": " << str << "\n";
   }
};

