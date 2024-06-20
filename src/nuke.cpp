#include "nuke.h"
#include "utils.h"
#include "app.h"
#include "request.h"
#include "response.h"

extern zclass* app_class;
extern zclass* res_class;
extern zclass* req_class;

zclass* FileKlass;

zobject init()
{
  zmodule* m = vm_alloc_zmodule();
  
  app_class = vm_alloc_zclass();
  app_class->name = "app";
  zclass_add_sig_method(app_class,"__construct__",&APP_CONSTRUCT,"");
  zclass_add_sig_method(app_class,"run",&APP_RUN,"osi");
  zclass_add_sig_method(app_class,"route",&APP_ROUTE,"ossw");
  zclass_add_method(app_class,"__del__",&APP_DEL);
  

  res_class = vm_alloc_zclass();
  res_class->name = "response";
  zclass_add_method(res_class,"__construct__",&RES_CONSTRUCT);

  req_class = vm_alloc_zclass();
  req_class->name = "request";
  zclass_add_sig_method(req_class,"getenv",&GetEnv,"os");
  zclass_add_sig_method(req_class,"args",&GetArgs,"o");
  zclass_add_sig_method(req_class,"json",&json,"o");
  zclass_add_sig_method(req_class,"data",&data,"o");
  

  zclass_add_sig_method(req_class,"form",&Form,"ob");
  
  FileKlass = vm_alloc_zclass();
  FileKlass->name = "File";
  vm_mark_important(FileKlass); 
  

  zmodule_add_class(m,"app",app_class);
  zmodule_add_class(m,"response",res_class);
  zmodule_add_class(m,"request",req_class);
  
  return zobj_from_module(m);
}
void unload()
{
  vm_unmark_important(FileKlass);
}
