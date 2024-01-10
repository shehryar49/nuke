#include "nuke.h"
#include "utils.h"
#include "app.h"
#include "request.h"
#include "response.h"

extern Klass* appKlass;
extern Klass* resKlass;
extern Klass* reqKlass;
Klass* FileKlass;
ZObject nil;

ZObject init()
{
  nil.type = Z_NIL;
  Module* m = vm_allocModule();
  
  appKlass = vm_allocKlass();
  appKlass->name = "app";
  Klass_addSigNativeMethod(appKlass,"__construct__",&APP_CONSTRUCT,"");
  Klass_addSigNativeMethod(appKlass,"run",&APP_RUN,"osi");
  Klass_addSigNativeMethod(appKlass,"route",&APP_ROUTE,"ossw");
  Klass_addNativeMethod(appKlass,"__del__",&APP_DEL);
  

  resKlass = vm_allocKlass();
  resKlass->name = "response";
  Klass_addNativeMethod(resKlass,"__construct__",&RES_CONSTRUCT);

  reqKlass = vm_allocKlass();
  reqKlass->name = "request";
  Klass_addMember(reqKlass,"cookies",nil);
  Klass_addSigNativeMethod(reqKlass,"getenv",&GetEnv,"os");
  Klass_addSigNativeMethod(reqKlass,"args",&GetArgs,"o");
  Klass_addSigNativeMethod(reqKlass,"form",&Form,"ob");
  
  FileKlass = vm_allocKlass();
  FileKlass->name = "File";
  vm_markImportant(FileKlass); 

  Module_addKlass(m,"app",appKlass);
  Module_addKlass(m,"response",resKlass);
  Module_addKlass(m,"request",reqKlass);
  
  return ZObjFromModule(m);
}
void unload()
{
  vm_unmarkImportant(FileKlass);
}
