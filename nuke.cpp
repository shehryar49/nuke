#include "nuke.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Klass* appKlass;
Klass* resKlass;
PltObject nil;
struct route //for dynamic routes (involving variables)
{

};
PltObject init()
{
  nil.type = PLT_NIL;
  Module* m = vm_allocModule();

  appKlass = vm_allocKlass();
  appKlass->name = "app";
  appKlass->members.emplace("run",PObjFromMethod("run",&APP_RUN,appKlass));
  appKlass->members.emplace("route",PObjFromMethod("route",&APP_ROUTE,appKlass));

  resKlass = vm_allocKlass();
  resKlass->name = "response";
  resKlass->members.emplace("__construct__",PObjFromMethod("__construct__",&RES_CONSTRUCT,resKlass));


  m->members.emplace("app",PObjFromKlass(appKlass));
  m->members.emplace("response",PObjFromKlass(resKlass));
  return PObjFromModule(m);
}
//Response methods
PltObject RES_CONSTRUCT(PltObject* args,int32_t n)
{
  if(n!=3)
    return Plt_Err(ArgumentError,"2 arguments required!");
  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != resKlass)
    return Plt_Err(TypeError,"'self' must be an object of response class.");
  if(args[1].type!=PLT_STR || args[2].type!=PLT_STR)
    return Plt_Err(TypeError,"Arguments must be strings!");
  KlassObject* self = (KlassObject*)args[0].ptr;
  self->members.emplace(".type",args[1]);
  self->members.emplace(".content",args[2]);
  return nil;
}
//App methods
PltObject APP_ROUTE(PltObject* args,int32_t n)
{
  if(n!=4)
    return Plt_Err(ArgumentError,"3 argument needed!");

  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != appKlass)
    return Plt_Err(TypeError,"'self' must be an object of app class.");
  if(args[1].type!=PLT_STR || args[2].type!=PLT_STR)
    return Plt_Err(TypeError,"Argument 1 and 2 must be strings!");
  if(args[3].type!=PLT_FUNC)
    return Plt_Err(TypeError,"Argument 3 must be a callback function!");
  FunObject* fun = (FunObject*)args[3].ptr;
  if(fun -> opt.size()!=0)
    return Plt_Err(ValueError,"Callback function must not take any optional args!");

  const string& method = *(string*)args[1].ptr;
  const string& path = *(string*)args[2].ptr;
  size_t len = path.size();
  for(size_t i=0;i<len;i++)
  {
    if(path[i] == '<')
    {
      for(size_t j=i+1;j<len && path[j]!='>';j++)
      {
        if()
      }
    }
  }
  KlassObject* self = (KlassObject*)args[0].ptr;
  if(method == "get")
    self->members.emplace(".get"+(string)path,args[3]);    
  else
    return Plt_Err(ValueError,"Unknown request method!");

  return nil;
}
PltObject APP_RUN(PltObject* args,int32_t n)
{
  if(n!=3)
    return Plt_Err(ArgumentError,"2 arguments needed!");
  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != appKlass)
    return Plt_Err(TypeError,"'self' must be an object of app class.");
  if(args[1].type!=PLT_STR)
    return Plt_Err(TypeError,"Argument 1 must be a string!");
  if(args[2].type!=PLT_INT)
    return Plt_Err(TypeError,"Argument 2 must be an integer!");
  if(args[2].i <= 0)
    return Plt_Err(ValueError,"Argument 2 must be a positive integer!");
  int32_t maxConnections = args[2].i;
  const string& ip = *(string*)args[1].ptr;
  std::string tmp;
  KlassObject* self = (KlassObject*)args[0].ptr;
  FCGX_Init();
  int sock = FCGX_OpenSocket(ip.c_str(),args[2].i);
  FCGX_Request req;
  int count = 0;
  std::unordered_map<string,PltObject>::iterator it;
  FCGX_InitRequest(&req,sock,0);
  printf("[+] Server started at %s\n",ip.c_str());
  while(FCGX_Accept_r(&req) >= 0)
  {
    char* method = FCGX_GetParam("REQUEST_METHOD",req.envp);
    char* addr = FCGX_GetParam("REMOTE_ADDR",req.envp);
    char* path = FCGX_GetParam("SCRIPT_NAME",req.envp);
    path += 5;
    tmp = path;
    printf("[+] Serving %s to host %s\n",tmp.c_str(),addr);

    if(strcmp(method,"GET") == 0)
    {
      if(tmp.back() == '/')
        tmp.pop_back();
      if(tmp == "")
        tmp.push_back('/');
      tmp = (string)".get"+tmp;
      printf("tmp = %s\n",tmp.c_str());
      if((it = self->members.find(tmp))!=self->members.end())
      {
        PltObject callback = (*it).second;
        PltObject rr;
        bool good = vm_callObject(&callback,NULL,0,&rr);
        if(!good || rr.type!=PLT_OBJ || ((KlassObject*)rr.ptr)->klass!=resKlass)
        {
          printf("Callback: %s\n",((string*)(((KlassObject*)rr.ptr)->members["msg"].ptr))->c_str());
          printf("[-] Invalid response by callback. Sending Status 500\n");
          FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\n");
        }
        else
        {
          KlassObject* obj = (KlassObject*)rr.ptr;
          const string& type = *(string*)obj->members[".type"].ptr;
          const string& content = *(string*)obj->members[".content"].ptr;
          FCGX_FPrintF(req.out,"Content-type: %s\r\n\r\n%s",type.c_str(),content.c_str());
        }
      }
      else
      {
        printf("[-] Unhandled method %s requested by host. Sending Status 500\n",method);
        FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\n");
      }
    }
    else
    {
      printf("[-] Unhandled method %s requested by host. Sending Status 500\n",method);
      FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\n");
    }
    FCGX_Finish_r(&req);
  }
  return nil;
}
