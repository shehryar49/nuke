#ifndef APP_H_
#define APP_H_
#include "nuke.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <fcgiapp.h>

Klass* appKlass;

extern "C"
{
    //App methods
    EXPORT PltObject APP_CONSTRUCT(PltObject*,int32_t);
    EXPORT PltObject APP_ROUTE(PltObject*,int32_t);
    EXPORT PltObject APP_RUN(PltObject*,int32_t);  
}
struct route //for dynamic routes (involving variables)
{
  std::vector<vector<string>> parts;
  std::vector<vector<bool>> dyn;
  std::vector<PltObject> callbacks;
  std::vector<string> reqMethods;
};


// Implementations

extern PltObject nil;
extern Klass* appKlass;
extern Klass* resKlass;

//App methods
PltObject APP_CONSTRUCT(PltObject* args,int32_t n)
{
  if(n!=1)
    return Plt_Err(ArgumentError,"0 arguments needed!");

  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != appKlass)
    return Plt_Err(TypeError,"'self' must be an object of app class.");

  KlassObject* self = (KlassObject*)args[0].ptr;
  route* r = new route;
  self->members.emplace(".routetable",PObjFromPtr((void*)r));
  return nil; 
}
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
  KlassObject* self = (KlassObject*)args[0].ptr;
  route* r = (route*)self->members[".routetable"].ptr;
  string path = AS_STR(args[2]);
  if(path.length() == 0)
    return Plt_Err(ValueError,"Empty string passed!");
  if(path[0]!='/')
    return Plt_Err(ValueError,"Error path must begin with a '/' ");
  if(path.back() == '/' && path.length()!=1)
    path.pop_back();
  size_t len = path.size();
  bool hasParam = false;
  size_t start = 0;
  string tmp;
  char last = 0;
  for(size_t i=0;i<len;i++)
  {
    if(path[i] == '<')
    {
      bool found = false;
      size_t idx = 0;
      for(size_t j=i+1;j<len;j++)
      {
        if(path[j] == '>')
        {
          found = true;
          idx = j;
          break;
        }
        else if(isalpha(path[j]) || isdigit(path[j]))
        ;
        else
          return Plt_Err(ValueError,(string)"Illegal character in path parameter name!");
      }
      if(!found)
        return Plt_Err(Error,"Invalid '<' is unmatched");
      if(!hasParam)
      {
        r->parts.push_back(vector<string>{});
        r->dyn.push_back(vector<bool>{});
      }
      hasParam = true;
      tmp = path.substr(start,i - start);
      if(tmp == "/") ;
      else
      {
        if(tmp.length()>=1 && tmp[0] == '/')
          tmp.erase(tmp.begin());
        if(tmp.length()>=1 && tmp.back() == '/')
          tmp.pop_back();
        r->dyn.back().push_back(false);
        r->parts.back().push_back(tmp); //before
      }

      r->dyn.back().push_back(true);
      r->parts.back().push_back(path.substr(i+1,idx-(i+1)));
      
      start = idx+1;
    }
    last = path[i];
  }
  
/*  if(hasParam)
  {
    size_t k=0;
    for(auto e: r->parts.back())
    {
      if(r->dyn.back()[k])
        printf("dynamic  ");
      puts(e.c_str());
      k++;
    }
  }*/
  if(method != "GET" && method!="POST" && method!="PUT" && method!="DELETE")
    return Plt_Err(ValueError,"Unknown request method!");
  if(!hasParam) // absolute path
    self->members.emplace(method+(string)path,args[3]);
  else //dynamic path with parameters
  {
    r->callbacks.push_back(args[3]);
    r->reqMethods.push_back(method);
  }
  vm_markImportant(args[3].ptr);
  return nil;
}
void handleCB(FCGX_Request& req,vector<PltObject>& args,PltObject callback)
{
  PltObject rr;
  PltObject* arr = (args.size() == 0) ? NULL : &args[0];
  bool good = vm_callObject(&callback,arr,(int32_t)args.size(),&rr);
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
    return Plt_Err(ValueError,"Argument 2 must be a positive and non zero integer!");
  int32_t maxConnections = AS_INT(args[2]);
  const string& ip = *(string*)args[1].ptr;
  std::string tmp;
  std::string tmp2;
  vector<string> parts;
  vector<PltObject> A;
  KlassObject* self = (KlassObject*)args[0].ptr;
  route* r = (route*)self->members[".routetable"].ptr;
  FCGX_Init();
  int sock = FCGX_OpenSocket(ip.c_str(),maxConnections);
  FCGX_Request req;
  int count = 0;
  std::unordered_map<string,PltObject>::iterator it;
  KlassObject reqObj;
  reqObj.klass = reqKlass;
  reqObj.members = reqKlass->members;
  reqObj.privateMembers = reqKlass->privateMembers;
  reqObj.members[".ptr"] = PObjFromPtr((void*)&req);
  Dictionary* argDict = vm_allocDict();
  reqObj.members["args"] = PObjFromDict(argDict);
  vm_markImportant(argDict);

  FCGX_InitRequest(&req,sock,0);
  
  printf("[+] Server started at %s\n",ip.c_str());
  while(FCGX_Accept_r(&req) >= 0)
  {
    char* method = FCGX_GetParam("REQUEST_METHOD",req.envp);
    char* addr = FCGX_GetParam("REMOTE_ADDR",req.envp);
    char* path = FCGX_GetParam("SCRIPT_NAME",req.envp);
    path += 5;//skip /nuke from start of url
    tmp = path;
    printf("[+] Serving %s to host %s\n",tmp.c_str(),addr);
  
    if(tmp.length()!=1 && tmp.back() == '/')
      tmp.pop_back();
    tmp2 = (string)method+tmp;
    //Check for absolute route
    if((it = self->members.find(tmp2))!=self->members.end())
    {
      PltObject callback = (*it).second;
      PltObject rr;
      A.clear();
      A.push_back(PObjFromKlassObj(&reqObj));
      argDict->clear();
      char* qstr = FCGX_GetParam("QUERY_STRING",req.envp);
      if(strcmp(method,"GET") == 0 && qstr)
      {
        vector<string> pairs = split((string)qstr,"&");
        for(auto pair: pairs)
        {
          vector<string> eq = split(pair,"=");
          if(eq.size()!=2)
             break;
          eq[0] = url_decode(eq[0]);
          eq[1] = url_decode(eq[1]);
          argDict->emplace(PObjFromStr(eq[0]),PObjFromStr(eq[1]));
        }
      }
      handleCB(req,A,callback);
    }
    else
    {
      if(tmp.size() > 1)
        tmp.erase(tmp.begin());
      if(tmp.size() > 1 && tmp.back() == '/')
        tmp.pop_back();
      // printf("[+] Trying dynamic routes\n");
      //check for dynamic routes
      parts = split(tmp,"/");
      size_t l = r->parts.size();
      bool handled = false;
      for(size_t i=0;i<l;i++)
      {
        if(r->parts[i].size() == parts.size() && r->reqMethods[i] == method)
        {
          size_t len = parts.size();
          bool match = true;
          vector<PltObject> args;
          args.push_back(PObjFromKlassObj(&reqObj));
          for(size_t j=0;j<len;j++)
          {
            if(r->parts[i][j] != parts[j])
            {
              if(r->dyn[i][j] && isValidParamVal(parts[j])) //a dynamic part
              {
                args.push_back(PObjFromStr(parts[j]));
              }
              else
              {
                match = false;
                break;
              }
            }
          }
          if(match)
          {
            printf("[+] Handling dynamic route request with callback!\n");
            handleCB(req,args,r->callbacks[i]);
            handled = true;
            break;
          }
        }
      }
      if(!handled)
      {
        printf("[-] Unhandled path %s %s requested by host. Sending Status 500\n",method,path);
        FCGX_FPrintF(req.out,"Content-type: text/html\r\nStatus: 500\r\n\r\n");
      }
    }
    
     
    FCGX_Finish_r(&req);
  }
  return nil;
}

PltObject APP_DEL(PltObject* args,int32_t n)
{
  if(n!=1)
    return Plt_Err(ArgumentError,"0 arguments needed!");
  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != appKlass)
    return Plt_Err(TypeError,"'self' must be an object of app class.");
  KlassObject* self = (KlassObject*)args[0].ptr;
  if(self->members[".routetable"].type == PLT_POINTER)
  {
    route* r = (route*)self->members[".routetable"].ptr;
    delete r;
  }
  return nil;
}
#endif