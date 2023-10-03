#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include "message.h"
#include "smartLockGuard.h"

template<class T>
class MessagesBroker 
{
   std::thread broker;
   std::map<int, T*> subscribersMap;
   std::queue<Message> msgsQueue;  
   bool isMessage;
   bool* stopCond;
   std::mutex messageMutex;

void BrokerRoutine()
{
   while(!(*stopCond))
   {
      if (isMessage)
      {
         smart_lock_guard<std::mutex> guard(isMessage, false, messageMutex);
         int queueSize = msgsQueue.size();
         for (int i = 0; i < queueSize;i++)
         {
            for(auto& element: subscribersMap)
            {
               element.second->SendMessage(msgsQueue.front());
            }
            msgsQueue.pop();  
         }
      }   
   }
}
public:

void IsMessageToSend(const Message* msg)
{
   smart_lock_guard<std::mutex> guard(isMessage, true, messageMutex);  
   msgsQueue.push(*msg);
}

MessagesBroker(bool* stopCond)
{  
   this->stopCond = stopCond;
   broker = std::thread(&MessagesBroker::BrokerRoutine, this);  
}

void AddSubscriber(const int socketNum, T* subscriber)
{
   subscribersMap.insert({socketNum, subscriber});
}

void EraseSubscriber(int key)
{
   delete subscribersMap[key];
   subscribersMap.erase(key);
}

void JoinRoutineThread()
{
   broker.join();
}

void TerminateTheSubscriberThread(int key);

void TerminateAllSubscribersThreads();
};
