#include "request.h"
#include "multipart.h"

zclass* req_class;
zclass* file_class;
//Implementations
zobject GetEnv(zobject* args,int32_t n)
{
  zclass_object* self = AS_ClASSOBJ(args[0]);
  if(self->_klass != req_class)
    return z_err(TypeError,"Error self must be an object of request class!");
  
  FCGX_Request* req = (FCGX_Request*)AS_PTR(zclassobj_get(self,".ptr"));
  zstr* name = AS_STR(args[1]); 
  char* val = FCGX_GetParam(name->val,req->envp);
  if(!val)
    return zobj_nil();
  return zobj_from_str(val);
}
zobject GetArgs(zobject* args,int32_t n) //parses QUERY_STRING and returns it
{
  zclass_object* self = AS_ClASSOBJ(args[0]);
  if(self->_klass != req_class)
    return z_err(TypeError,"Error self must be an object of request class!");
  
  FCGX_Request* req = (FCGX_Request*)AS_PTR(zclassobj_get(self,".ptr"));
  char* qstr = FCGX_GetParam("QUERY_STRING",req->envp);
  if(qstr)
  {
    zdict* argDict = vm_alloc_zdict();
    if(strcmp(qstr,"") == 0)
      return zobj_from_dict(argDict);
    vector<string> pairs = split((string)qstr,'&');
    for(auto pair: pairs)
    {
      vector<string> eq = split(pair,"=");
      if(eq.size()!=2)
        return z_err(ValueError,"Bad or unsupported format of QUERY_STRING. Unable to parse!");
      eq[0] = url_decode(eq[0]);
      eq[1] = url_decode(eq[1]);
      zdict_emplace(argDict,zobj_from_str(eq[0].c_str()),zobj_from_str(eq[1].c_str()));
    }
    return zobj_from_dict(argDict);
  }
  return zobj_nil();
}
zdict* parse_multipart(char*,size_t,const string&,bool);
zobject Form(zobject* args,int32_t n) //parses POST request and returns it
{
  zclass_object* self = AS_ClASSOBJ(args[0]);
  if(self->_klass != req_class)
    return z_err(TypeError,"Error self must be an object of request class!");
  bool defaultText = AS_BOOL(args[1]);
  FCGX_Request* req = (FCGX_Request*)AS_PTR(zclassobj_get(self,".ptr"));
  char* method = FCGX_GetParam("REQUEST_METHOD",req->envp);
  
  if(!method || strcmp(method,"POST")!=0)
    return z_err(Error,"No form POSTed!");
  
  char* contentType = FCGX_GetParam("CONTENT_TYPE",req->envp);
  if(!contentType)
    return z_err(Error,"Error environment variable CONTENT_TYPE not found!");
  
  if(strcmp(contentType,"application/x-www-form-urlencoded") == 0)
  {
    char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
    if(!contentLen)
      return z_err(Error,"CONTENT_LENGTH variable not found!");
    size_t len = atoll(contentLen);
    string payload;
    payload.resize(len+1);
    size_t i = 1;
    while(i<=len)
    {
      payload[i-1] = FCGX_GetChar(req->in);
      i+=1;
    }
    zdict* argDict = vm_alloc_zdict();
    if(payload.length() == 0)
      return zobj_from_dict(argDict);
    vector<string> pairs = split(payload,'&');
    for(auto pair: pairs)
    {
      vector<string> eq = split(pair,"=");
      if(eq.size()!=2)
        return z_err(ValueError,"Bad or unsupported format. Unable to parse!");
      eq[0] = url_decode(eq[0]);
      eq[1] = url_decode(eq[1]);
      zdict_emplace(argDict,zobj_from_str(eq[0].c_str()),zobj_from_str(eq[1].c_str()));
    }
    return zobj_from_dict(argDict);    
  }
  else if(strncmp(contentType,"multipart/form-data",18) == 0) // a multipart form
  {
    //this gets interesting
    //and also messy
    size_t k = 19;
    while(contentType[k] == ' ') // there is a null terminator, so no out of bounds problem
      k++;
    if(contentType[k] != ';')
      return z_err(Error,"Invalid format of CONTENT_TYPE. Unable to parse!");
    k++;
    while(contentType[k] == ' ') // there is a null terminator, so out bounds problem
      k++;
    if(contentType[k] == 0 || strncmp(contentType+k,"boundary=",9) != 0)
      return z_err(Error,"Invaid format of CONTENT_TYPE. Unable to parse!");
    //all this non sense because stupid HTTP chose to use 'optional' space
    string boundary = contentType+k+9;
    if(boundary.length() == 0)
      return z_err(Error,"Invalid format of CONTENT_TYPE. Unable to parse!");
    char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
    if(!contentLen)
      return z_err(Error,"CONTENT_LENGTH variable not found!");
    size_t len = atoll(contentLen);
    if(len == 0)
      return z_err(ValueError,"POSTed data length is 0. wtf?");
    char* payload = new char[len+1];
    size_t i = 1;
    while(i<=len)
    {
      payload[i-1] = FCGX_GetChar(req->in);
      i+=1;
    }
    payload[len] = 0;
    multipart_parser parser(payload,len,boundary);
    try
    {
      zdict* dict = parser.parse();
      return zobj_from_dict(dict);
    }
    catch(const parse_error& err)
    {
      return z_err(Error,"Error parsing multipart form. Bad or unsupported format!");
    }
  }
  else
    return z_err(Error,"Unknown content-type used for form!");
  return zobj_nil();
}
zobject json(zobject* args,int32_t n) //checks if content/type is applicaton/json
{
  zclass_object* self = AS_ClASSOBJ(args[0]);
  if(self->_klass != req_class)
    return z_err(TypeError,"Error self must be an object of request class!");
  FCGX_Request* req = (FCGX_Request*)AS_PTR(zclassobj_get(self,".ptr"));
  char* method = FCGX_GetParam("REQUEST_METHOD",req->envp);
  
  if(!method || (strcmp(method,"POST")!=0 && strcmp(method,"PUT")!=0))
    return z_err(Error,"No data POSTed or PUTed!");
  
  char* contentType = FCGX_GetParam("CONTENT_TYPE",req->envp);
  if(!contentType)
    return z_err(Error,"Error environment variable CONTENT_TYPE not found!");
  
  if(strcmp(contentType,"application/json") == 0)
  {
  
    char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
    if(!contentLen)
      return z_err(Error,"CONTENT_LENGTH variable not found!");
    size_t len = atoll(contentLen);
    zstr* payload = vm_alloc_zstr(len);
    size_t i = 1;
    while(i<=len)
    {
      payload->val[i-1] = FCGX_GetChar(req->in);
      i+=1;
    }
    return zobj_from_str_ptr(payload);
  }
  return zobj_nil();
}
zobject data(zobject* args,int32_t n) 
{
  zclass_object* self = AS_ClASSOBJ(args[0]);
  if(self->_klass != req_class)
    return z_err(TypeError,"Error self must be an object of request class!");
  FCGX_Request* req = (FCGX_Request*)AS_PTR(zclassobj_get(self,".ptr"));
  char* method = FCGX_GetParam("REQUEST_METHOD",req->envp);
  
  if(!method || (strcmp(method,"POST")!=0 && strcmp(method,"PUT")!=0))
    return z_err(Error,"No data POSTed or PUTed!");
  
  char* contentType = FCGX_GetParam("CONTENT_TYPE",req->envp);
  if(!contentType)
    return z_err(Error,"Error environment variable CONTENT_TYPE not found!");
  
  char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
  if(!contentLen)
    return z_err(Error,"CONTENT_LENGTH variable not found!");
  size_t len = atoll(contentLen);
  zbytearr* payload = vm_alloc_zbytearr();
  size_t i = 1;
  while(i<=len)
  {
    zbytearr_push(payload,FCGX_GetChar(req->in));
    i+=1;
  }
  return zobj_from_bytearr(payload);
  
}
