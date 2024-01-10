//Ladies and Gentleman I present you the "app"
#ifndef APP_H_
#define APP_H_
#include "nuke.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <string>
#include <fcgiapp.h>
#include "utils.h"
using namespace std;

Klass* appKlass;

extern "C"
{
    //App methods
    EXPORT ZObject APP_CONSTRUCT(ZObject*,int32_t);
    EXPORT ZObject APP_ROUTE(ZObject*,int32_t);
    EXPORT ZObject APP_RUN(ZObject*,int32_t);  
}
struct route //for dynamic routes (involving variables)
{
  std::vector<vector<string>> parts;
  std::vector<vector<bool>> dyn;
  std::vector<ZObject> callbacks;
  std::vector<string> reqMethods;
};


// Implementations

extern ZObject nil;
extern Klass* appKlass;
extern Klass* resKlass;

//App methods
ZObject APP_CONSTRUCT(ZObject* args,int32_t n)
{
  KlassObject* self = (KlassObject*)args[0].ptr;
  if(self->klass != appKlass)
    return Z_Err(TypeError,"'self' must be an object of app class.");
  route* r = new route;
  KlassObj_setMember(self,".routetable",ZObjFromPtr((void*)r)); 
  return nil; 
}
ZObject APP_ROUTE(ZObject* args,int32_t n)
{
  KlassObject* self = (KlassObject*)args[0].ptr;
  if(self->klass != appKlass)
    return Z_Err(TypeError,"'self' must be an object of app class.");
  FunObject* fun = (FunObject*)args[3].ptr;
  if(fun -> opt.size!=0)
    return Z_Err(ValueError,"Callback function must not take any optional args!");

  string method = AS_STR(args[1])->val;
  route* r = (route*)AS_PTR(KlassObj_getMember(self,".routetable"));
  string path = AS_STR(args[2])->val;
  
  if(path.length() == 0)
    return Z_Err(ValueError,"Empty path passed!");
  if(path[0]!='/')
    return Z_Err(ValueError,"Error path must begin with a '/' ");
  
  vector<string> parts = split(path,'/');
  //first part is always empty becuase of the '/' at the beginning
  parts.erase(parts.begin());

  bool isdynamic = false;
  vector<bool> dyn;

  for(auto& part: parts)
  {
    if(part.length() == 0)
      return Z_Err(ValueError,"Invalid path syntax");
    if(part.length() >= 2 && part[0] == '<' && part.back() == '>')
    {
      part.erase(part.begin());
      part.pop_back();
      if(!isalphanum(part))
        return Z_Err(ValueError,"Dynamic path variable must be alphanumeric!");
      isdynamic = true;
      dyn.push_back(true);
    }
    else
    {
      if(!isValidUrlPart(part))
        return Z_Err(ValueError,"Invalid path syntax");
      dyn.push_back(false);
    }
  }
  //
  if(isdynamic)
  {
    r->parts.push_back(parts);
    r->dyn.push_back(dyn);
    r->callbacks.push_back(args[3]);
    r->reqMethods.push_back(method);
  }
  else
  {
    string str = method + path;
    ZObject p = ZObjFromStr(str.c_str());
    vm_markImportant(p.ptr); // won't be required in the future
    KlassObj_setMember(self,AS_STR(p)->val,args[3]);
  }
  //mark the callback important
  // so the vm doesn't free it
  vm_markImportant(args[3].ptr);
  return nil;
}
void handleCB(FCGX_Request& req,ZObject* args,int32_t n,ZObject callback)
{
  ZObject rr;
  bool good = vm_callObject(&callback,args,(int32_t)n,&rr);
  if(!good)
  {
    ZObject msg = KlassObj_getMember((KlassObject*)rr.ptr,"msg");
    printf("Callback: %s\n",AS_STR(msg)->val);
    printf("[-] Invalid response by callback. Sending Status 500\n");
    FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\n");
  }
  else if(rr.type!=Z_OBJ || ((KlassObject*)rr.ptr)->klass!=resKlass)
  {
    puts("[-] Response returned by callback is not an object of nuke.response class!");
    puts("[-] Sending status 500 to client");
    FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\nAn internal server error ocurred");
  }
  else
  {
    KlassObject* obj = (KlassObject*)rr.ptr;
    const char* type = AS_STR(KlassObj_getMember(obj,".type"))->val;
    int status = AS_INT(KlassObj_getMember(obj,".status"));
    ZObject content = KlassObj_getMember(obj,".content");
    if(content.type == Z_STR)
    {
      FCGX_FPrintF(req.out,"Content-type: %s\r\nStatus: %d\r\n\r\n%s",type,status,AS_STR(content)->val);
    }
    else
    {
      FCGX_FPrintF(req.out,"Content-type: %s\r\nStatus: %d\r\n\r\n",type,status);
      auto bt = AS_BYTEARRAY(content);
      FCGX_PutStr((char*)(bt->arr),bt->size*sizeof(uint8_t),req.out);
    }
  }
}
ZObject APP_RUN(ZObject* args,int32_t n)
{
  KlassObject* self = (KlassObject*)args[0].ptr;
  if(self->klass != appKlass)
    return Z_Err(TypeError,"'self' must be an object of app class.");
  if(args[2].i <= 0)
    return Z_Err(ValueError,"Argument 2 must be a positive and non zero integer!");

  int32_t maxConnections = AS_INT(args[2]);
  const char* ip = AS_STR(args[1])->val;
  std::string tmp;
  std::string tmp2;
  vector<string> parts;
  vector<ZObject> A;

  //dynamic route table
  route* routeTable = (route*)AS_PTR(KlassObj_getMember(self,".routetable"));

  FCGX_Init();
  int sock = FCGX_OpenSocket(ip,maxConnections);
  FCGX_Request req;
 
  KlassObject* reqObj = vm_allocKlassObject(reqKlass);
  vm_markImportant(reqObj); //same request object used over and over again
 
  ZDict* argDict = vm_allocDict();
  KlassObj_setMember(reqObj,".ptr",ZObjFromPtr((void*)&req));

  vm_markImportant(argDict);

  FCGX_InitRequest(&req,sock,0);
  
  printf("[+] Server started at %s\n",ip);
  ZObject reqArg = ZObjFromKlassObj(reqObj);
  vector<ZObject> dynamicReqArgs;

  while(FCGX_Accept_r(&req) >= 0)
  {
    char* method = FCGX_GetParam("REQUEST_METHOD",req.envp);
    char* path = FCGX_GetParam("SCRIPT_NAME",req.envp);
    path += 5;//skip /nuke from start of url
    tmp = path;
    //printf("[+] Serving %s to host %s\n",tmp.c_str(),addr);
  
    if(tmp.length()!=1 && tmp.back() == '/')
      tmp.pop_back();
    tmp2 = (string)method+tmp;
    //Check for absolute route
    ZObject callback;
    if(StrMap_get(&(self->members),tmp2.c_str(),&callback))
    {
      handleCB(req,&reqArg,1,callback);
    }
    else
    {
      //check for dynamic routes
      if(tmp[0] == '/')
        tmp.erase(tmp.begin());
      parts = split(tmp,'/');
      
      size_t l = routeTable->parts.size();
      bool handled = false;
      for(size_t i=0;i<l;i++) //ignore first part it is always empty
      {
        if(routeTable->parts[i].size() == parts.size() && routeTable->reqMethods[i] == method)
        {
          size_t len = parts.size();
          bool match = true;
          dynamicReqArgs.clear();
          dynamicReqArgs.push_back(ZObjFromKlassObj(reqObj));
          for(size_t j=0;j<len;j++)
          {
            if(routeTable->dyn[i][j]) //a dynamic part
            {
              dynamicReqArgs.push_back(ZObjFromStr(parts[j].c_str()));
            }
            else if(routeTable->parts[i][j] != parts[j]) //is static
            {
              //static part mismatched
              match = false;
              break;
            }
            //part is either static or dynamic, only two possibilities
            //so no else needed
          }
          if(match)
          {
            handleCB(req,&dynamicReqArgs[0],dynamicReqArgs.size(),routeTable->callbacks[i]);
            handled = true;
            break;
          }
        }
      }
      if(!handled)
      {
        printf("[-] Unhandled path %s %s requested by host. Sending Status 404\n",method,path);
        FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 404\r\n\r\n");
      }
    }
    
     
    FCGX_Finish_r(&req);
  }
  return nil;
}

ZObject APP_DEL(ZObject* args,int32_t n)
{
  if(n!=1)
    return Z_Err(ArgumentError,"0 arguments needed!");
  if(args[0].type != Z_OBJ || ((KlassObject*)args[0].ptr)->klass != appKlass)
    return Z_Err(TypeError,"'self' must be an object of app class.");
  KlassObject* self = (KlassObject*)args[0].ptr;
  ZObject ptr = KlassObj_getMember(self,".routetable");
  if(ptr.type == Z_POINTER)
  {
    route* r = (route*)ptr.ptr;
    delete r;
  }
  return nil;
}
#endif
