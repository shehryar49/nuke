//Ladies and Gentleman I present you the "app"
#ifndef APP_H_
#define APP_H_
#include "nuke.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <fcgiapp.h>
#include "utils.h"
#ifdef _WIN32
  // no threading available on windows
  // nuke on windows should only be used for testing
  // only an idiot would use windows to run a server
  // switch to linux (the performance gains are riyal)
#else
  #include <pthread.h>
  #include <semaphore.h>
  #define NUKE_USE_THREADING         
#endif


#define NUM_THREADS 5



extern "C"
{
    //App methods
    EXPORT zobject APP_CONSTRUCT(zobject*,int32_t);
    EXPORT zobject APP_ROUTE(zobject*,int32_t);
    EXPORT zobject APP_RUN(zobject*,int32_t);  
    EXPORT zobject APP_DEL(zobject*,int32_t);
}
struct route //for dynamic routes (involving variables)
{
  std::vector<vector<string>> parts;
  std::vector<vector<bool>> dyn;
  std::vector<zobject> callbacks;
  std::vector<string> reqMethods;
};


// Implementations

extern zclass* app_class;
extern zclass* res_class;


#endif
