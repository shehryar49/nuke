#include "request.h"

zclass* req_class;
zclass* file_class;
//Implementations
zobject GetEnv(zobject* args,int32_t n)
{
  zclass_object* self = AS_KlASSOBJ(args[0]);
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
  zclass_object* self = AS_KlASSOBJ(args[0]);
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
  zclass_object* self = AS_KlASSOBJ(args[0]);
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
    bool hadErr = false;
    zdict* dict = parse_multipart(payload,len,boundary,defaultText);
    delete[] payload;
    if(dict)
      return zobj_from_dict(dict);
    else
      return z_err(Error,"Error parsing multipart form. Bad or unsupported format!");
  }
  else
    return z_err(Error,"Unknown content-type used for form!");
  return zobj_nil();
}
zobject json(zobject* args,int32_t n) //checks if content/type is applicaton/json
{
  zclass_object* self = AS_KlASSOBJ(args[0]);
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
zobject data(zobject* args,int32_t n) //checks if content/type is applicaton/json
{
  zclass_object* self = AS_KlASSOBJ(args[0]);
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
zdict* parse_multipart(char* data,size_t len,const string& boundary,bool defaultToText=false)
{
  if(len <=2 || strncmp(data,"--",2)!=0)
    return nullptr;
  
  size_t k = 2;
  if(strncmp(data+k,boundary.c_str(),boundary.length()) != 0)
    return nullptr;
 
  k += boundary.length();
 
  //--CRLF at the end of boundary
  if(k+3>=len || strncmp(data+k,"\r\n",2)!=0)
    return nullptr;
  //first boundary is valid
  k+=2;
  string headername;
  vector<string> values;
  string partname;
  string contentType;
  string filename;
  string content;
  zdict* payload = vm_alloc_zdict();
  while(k < len)
  {
    //process each part
    filename.clear();
    partname.clear();
    contentType.clear();
    content.clear();
    //read headers
    while(true)
    {
      headername.clear();
      bool readingName = true;
      bool inquotes = false;
      values = {""};
      while(k < len && data[k]!='\r')
      {
        if(data[k] == ':')
        {
          if(!readingName)
            return NULL;
          readingName = false;
        }
        else if(readingName)
          headername+=tolower(data[k]);
        else if(!readingName) //reading header value
        {
          if(data[k] == '"')
            inquotes = !inquotes;
          else
          {
            if(inquotes)
              values.back().push_back(data[k]);
            else if(data[k] == ' '); //ignore
            else if(data[k] == ';')
              values.push_back("");
            else
              values.back().push_back(data[k]); 
          }
        }  
        k++;
      }

      if(inquotes)
        return nullptr;
      if(k >= len || data[k]!='\r')
        return nullptr;
      k++;//consume CR
      if(k>=len || data[k]!='\n')
        return nullptr;
      k++;
      if(headername!="")
      {
        for(auto& value: values)
        {
          if(value.length() == 0)
            return nullptr;
        }
      }
      if(headername == "content-disposition")
      {

        if( values.size() == 2 &&
            values[0] == "form-data" &&
            values[1].length()>5 &&
            values[1].substr(0,5) == "name="
          )
          partname = values[1].substr(5);
        else if( values.size() == 3 &&
            values[0] == "form-data" &&
            values[1].length()>5 &&
            values[1].substr(0,5) == "name=" &&
            values[2].length() > 9 &&
            values[2].substr(0,9) == "filename="
          )
        {
          partname = values[1].substr(5);
          filename = values[2].substr(9);
        }  
        else
          return nullptr;
      }
      else if(headername == "content-type")
      {
        if(values.size()==1 && values[0].length()!=0) 
          contentType = values[0];
        else
          return nullptr;
      }
      else if(headername == "")// end of headers
        break;
      else
        return nullptr; //bad or unsupported header  
    }
    //return nullptr;
    //read content
    bool boundarymatched = false;
    content.clear();
    zbytearr* bt = NULL;
    bool useText = false;
    if(contentType == "text/plain" || (contentType=="" && defaultToText && filename==""))
    useText=true;
    else
      bt = vm_alloc_zbytearr();
    while (k<len)
    {

      if(data[k] == '\r' && k+3+boundary.length() < len && data[k+1]=='\n' &&
         strncmp(data+k+2,"--",2)==0 &&  
         strncmp(data+k+4,boundary.c_str(),boundary.length())==0
         ) //boundary match
      {
        boundarymatched = true;
        k = k+4+boundary.length();
        if(k+1 >= len)
          return nullptr;
        if(data[k] =='-' && data[k+1] == '-') //last boundary
        {
          k+=4;//consume CRLF
          
        }
        else if(data[k] == '\r' && data[k+1] == '\n')
          k+=2;
        else
          return nullptr;
        break;
      }
      else
      {
        if(contentType == "text/plain" || useText)
          content += data[k];
        else
          zbytearr_push(bt,data[k]);
      }
      k+=1;
    }
    if(!boundarymatched)
      return nullptr;
    if(contentType == "" && defaultToText && filename=="")
    {
        zdict_emplace(payload,zobj_from_str(partname.c_str()),zobj_from_str(content.c_str()));
    }
    else if(bt)
    {
      if(filename != "")
      {
        zclass_object* ko = vm_alloc_zclassobj(file_class);
        zclassobj_set(ko,"data",zobj_from_bytearr(bt));
        zclassobj_set(ko,"filename",zobj_from_str(filename.c_str()));
        if(contentType != "")
          zclassobj_set(ko,"contentType",zobj_from_str(contentType.c_str()));
        zdict_emplace(payload,zobj_from_str(partname.c_str()),zobj_from_classobj(ko));
      }
      else
        zdict_emplace(payload,zobj_from_str(partname.c_str()),zobj_from_bytearr(bt));
    } 
    else
    {
      if(filename != "")
      {
        zclass_object* ko = vm_alloc_zclassobj(file_class);
        zclassobj_set(ko,"data",zobj_from_str(content.c_str()));
        zclassobj_set(ko,"filename",zobj_from_str(filename.c_str()));
        if(contentType != "")
          zclassobj_set(ko,"contentType",zobj_from_str(contentType.c_str()));
        zdict_emplace(payload,zobj_from_str(partname.c_str()),zobj_from_classobj(ko));
      }
      else
        zdict_emplace(payload,zobj_from_str(partname.c_str()),zobj_from_str(content.c_str()));
    }         
  }
  return payload;
}