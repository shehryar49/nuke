#include "nuke.h"
#include "utils.h"
#include "app.h"
#include "request.h"
#include "response.h"

extern Klass* appKlass;
extern Klass* resKlass;
extern Klass* reqKlass;
ZObject nil;

ZObject init()
{
  nil.type = Z_NIL;
  Module* m = vm_allocModule();

  appKlass = vm_allocKlass();
  appKlass->name = "app";
  Klass_addNativeMethod(appKlass,"__construct__",&APP_CONSTRUCT);
  Klass_addNativeMethod(appKlass,"run",&APP_RUN);
  Klass_addNativeMethod(appKlass,"route",&APP_ROUTE);
  Klass_addNativeMethod(appKlass,"__del__",&APP_DEL);
  

  resKlass = vm_allocKlass();
  resKlass->name = "response";
  Klass_addNativeMethod(resKlass,"__construct__",&RES_CONSTRUCT);

  reqKlass = vm_allocKlass();
  reqKlass->name = "request";
  Klass_addMember(reqKlass,"cookies",nil);
  Klass_addMember(reqKlass,"args",nil);
  Klass_addMember(reqKlass,"form",nil);
  Klass_addNativeMethod(reqKlass,"getenv",&GetParam);
  
  

  Module_addKlass(m,"app",appKlass);
  Module_addKlass(m,"response",resKlass);
  Module_addKlass(m,"request",reqKlass);
  
  return ZObjFromModule(m);
}
