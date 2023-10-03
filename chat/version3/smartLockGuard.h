#include <mutex>

template <class T>
class smart_lock_guard : std::lock_guard<T> {
bool value;
bool& varToChange;
public: 
   smart_lock_guard(bool& varToChange, bool value, T& mutex) : std::lock_guard<T>(mutex), varToChange(varToChange) 
   {
      this->value = value;
   }
   
   ~smart_lock_guard()
   {
      varToChange = value;
   }
};
