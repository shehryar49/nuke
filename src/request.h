#ifndef REQUEST_H_
#define REQUEST_H_
#include "nuke.h"
#include "utils.h"
#include <fcgiapp.h>
#include <vector>
#include <string>
using namespace std;

extern zclass* req_class;
extern zclass* file_class;

extern "C"
{
  //Request methods
  zobject GetEnv(zobject*,int32_t);
  zobject GetArgs(zobject*,int32_t);
  zobject Form(zobject*,int32_t);
  zobject json(zobject*,int32_t);
  zobject data(zobject*,int32_t);
  
}
#endif
