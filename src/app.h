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
  #define NUM_THREADS 5       
#endif

extern zclass* app_class;

extern "C"
{
    //App methods
    EXPORT zobject app_construct(zobject*,int32_t);
    EXPORT zobject app_route(zobject*,int32_t);
    EXPORT zobject app_run(zobject*,int32_t);  
    EXPORT zobject app_del(zobject*,int32_t);
}
struct route //for dynamic routes (involving variables)
{
  std::vector<vector<string>> parts;
  std::vector<vector<bool>> dyn;
  std::vector<zobject> callbacks;
  std::vector<string> reqMethods;
};


#endif
