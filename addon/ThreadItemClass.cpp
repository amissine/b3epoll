#include "addon.h"
#include <iostream>

using namespace std;

class ThreadItemClass {
  public:
    ThreadItemClass () {
    }
    static ThreadItemClass* newThreadItem () {
      cout << "ThreadItemClass::newThreadItem started" << endl;
      return new ThreadItemClass();
    }
};

extern "C" ThreadItemClass* ThreadItemClass__newThreadItem () {
  return ThreadItemClass::newThreadItem();
}
