#include "nuke.h"
#include "utils.h"
#include "app.h"
#include "request.h"
#include "response.h"

extern Klass* appKlass;
extern Klass* resKlass;
extern Klass* reqKlass;
PltObject nil;


PltObject init()
{
  nil.type = PLT_NIL;
  Module* m = vm_allocModule();

  appKlass = vm_allocKlass();
  appKlass->name = "app";
  appKlass->members.emplace("__construct__",PObjFromMethod("__construct__",&APP_CONSTRUCT,appKlass));
  appKlass->members.emplace("run",PObjFromMethod("run",&APP_RUN,appKlass));
  appKlass->members.emplace("route",PObjFromMethod("route",&APP_ROUTE,appKlass));

  resKlass = vm_allocKlass();
  resKlass->name = "response";
  resKlass->members.emplace("__construct__",PObjFromMethod("__construct__",&RES_CONSTRUCT,resKlass));


  m->members.emplace("app",PObjFromKlass(appKlass));
  m->members.emplace("response",PObjFromKlass(resKlass));
  return PObjFromModule(m);
}
