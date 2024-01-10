#ifndef REQUEST_H_
#define REQUEST_H_
#include "nuke.h"
#include "utils.h"
#include <fcgiapp.h>
#include <vector>
#include <string>
using namespace std;

Klass* reqKlass;
extern ZObject nil;
extern Klass* FileKlass;
extern "C"
{
  //Request methods
  ZObject GetEnv(ZObject*,int32_t);
  ZObject GetArgs(ZObject*,int32_t);
  ZObject Form(ZObject*,int32_t);
}

//Implementations
ZObject GetEnv(ZObject* args,int32_t n)
{
  KlassObject* self = AS_KlASSOBJ(args[0]);
  if(self->klass != reqKlass)
    return Z_Err(TypeError,"Error self must be an object of request class!");
  
  FCGX_Request* req = (FCGX_Request*)AS_PTR(KlassObj_getMember(self,".ptr"));
  ZStr* name = AS_STR(args[1]); 
  char* val = FCGX_GetParam(name->val,req->envp);
  if(!val)
    return nil;
  return ZObjFromStr(val);
}
ZObject GetArgs(ZObject* args,int32_t n) //parses QUERY_STRING and returns it
{
  KlassObject* self = AS_KlASSOBJ(args[0]);
  if(self->klass != reqKlass)
    return Z_Err(TypeError,"Error self must be an object of request class!");
  
  FCGX_Request* req = (FCGX_Request*)AS_PTR(KlassObj_getMember(self,".ptr"));
  char* qstr = FCGX_GetParam("QUERY_STRING",req->envp);
  if(qstr)
  {
    ZDict* argDict = vm_allocDict();
    if(strcmp(qstr,"") == 0)
      return ZObjFromDict(argDict);
    vector<string> pairs = split((string)qstr,'&');
    for(auto pair: pairs)
    {
      vector<string> eq = split(pair,"=");
      if(eq.size()!=2)
        return Z_Err(ValueError,"Bad or unsupported format of QUERY_STRING. Unable to parse!");
      eq[0] = url_decode(eq[0]);
      eq[1] = url_decode(eq[1]);
      ZDict_emplace(argDict,ZObjFromStr(eq[0].c_str()),ZObjFromStr(eq[1].c_str()));
    }
    return ZObjFromDict(argDict);
  }
  return nil;
}
ZDict* parse_multipart(char*,size_t,const string&,bool);
ZObject Form(ZObject* args,int32_t n) //parses POST request and returns it
{
  KlassObject* self = AS_KlASSOBJ(args[0]);
  if(self->klass != reqKlass)
    return Z_Err(TypeError,"Error self must be an object of request class!");
  bool defaultText = AS_BOOL(args[1]);
  FCGX_Request* req = (FCGX_Request*)AS_PTR(KlassObj_getMember(self,".ptr"));
  char* method = FCGX_GetParam("REQUEST_METHOD",req->envp);
  
  if(!method || strcmp(method,"POST")!=0)
    return Z_Err(Error,"No form POSTed!");
  
  char* contentType = FCGX_GetParam("CONTENT_TYPE",req->envp);
  if(!contentType)
    return Z_Err(Error,"Error environment variable CONTENT_TYPE not found!");
  
  if(strcmp(contentType,"application/x-www-form-urlencoded") == 0)
  {
  
    char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
    if(!contentLen)
      return Z_Err(Error,"CONTENT_LENGTH variable not found!");
    size_t len = atoll(contentLen);
    string payload;
    payload.resize(len+1);
    FCGX_GetLine(&payload[0],len+1,req->in);
    ZDict* argDict = vm_allocDict();
    if(payload.length() == 0)
      return ZObjFromDict(argDict);
    vector<string> pairs = split(payload,'&');
    for(auto pair: pairs)
    {
      vector<string> eq = split(pair,"=");
      if(eq.size()!=2)
        return Z_Err(ValueError,"Bad or unsupported format. Unable to parse!");
      eq[0] = url_decode(eq[0]);
      eq[1] = url_decode(eq[1]);
      ZDict_emplace(argDict,ZObjFromStr(eq[0].c_str()),ZObjFromStr(eq[1].c_str()));
    }
    return ZObjFromDict(argDict);    
  }
  else if(strncmp(contentType,"multipart/form-data",18) == 0) // a multipart form
  {
    //this gets interesting
    //and also messy
    size_t k = 19;
    while(contentType[k] == ' ') // there is a null terminator, so no out of bounds problem
      k++;
    if(contentType[k] != ';')
      return Z_Err(Error,"Invalid format of CONTENT_TYPE. Unable to parse!");
    k++;
    while(contentType[k] == ' ') // there is a null terminator, so out bounds problem
      k++;
    if(contentType[k] == 0 || strncmp(contentType+k,"boundary=",9) != 0)
      return Z_Err(Error,"Invaid format of CONTENT_TYPE. Unable to parse!");
    //all this non sense because stupid HTTP chose to use 'optional' space
    string boundary = contentType+k+9;
    if(boundary.length() == 0)
      return Z_Err(Error,"Invalid format of CONTENT_TYPE. Unable to parse!");
    char* contentLen = FCGX_GetParam("CONTENT_LENGTH",req->envp);
    if(!contentLen)
      return Z_Err(Error,"CONTENT_LENGTH variable not found!");
    size_t len = atoll(contentLen);
    if(len == 0)
      return Z_Err(ValueError,"POSTed data length is 0. wtf?");
    char* payload = new char[len+1];
    size_t i = 1;
    while(i<=len)
    {
      payload[i-1] = FCGX_GetChar(req->in);
      i+=1;
    }
    payload[len] = 0;
    bool hadErr = false;
    ZDict* dict = parse_multipart(payload,len,boundary,defaultText);
    delete[] payload;
    if(dict)
      return ZObjFromDict(dict);
    else
      return Z_Err(Error,"Error parsing multipart form. Bad or unsupported format!");
  }
  else
    return Z_Err(Error,"Unknown content-type used for form!");
  return nil;
}
ZDict* parse_multipart(char* data,size_t len,const string& boundary,bool defaultToText=false)
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
  ZDict* payload = vm_allocDict();
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
            values[2] == "filename="
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
    ZByteArr* bt = NULL;
    bool useText = false;
    if(contentType == "text/plain" || (contentType=="" && defaultToText && filename==""))
    useText=true;
    else
      bt = vm_allocByteArray();
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
          ZByteArr_push(bt,data[k]);
      }
      k+=1;
    }
    if(!boundarymatched)
      return nullptr;
    if(contentType == "" && defaultToText && filename=="")
    {
        ZDict_emplace(payload,ZObjFromStr(partname.c_str()),ZObjFromStr(content.c_str()));
    }
    else if(bt)
    {
      if(filename != "")
      {
        KlassObject* ko = vm_allocKlassObject(FileKlass);
        KlassObj_setMember(ko,"data",ZObjFromByteArr(bt));
        KlassObj_setMember(ko,"filename",ZObjFromStr(filename.c_str()));
        ZDict_emplace(payload,ZObjFromStr(partname.c_str()),ZObjFromKlassObj(ko));
      }
      else
        ZDict_emplace(payload,ZObjFromStr(partname.c_str()),ZObjFromByteArr(bt));
    } 
    else
    {
      if(filename != "")
      {
        KlassObject* ko = vm_allocKlassObject(FileKlass);
        KlassObj_setMember(ko,"data",ZObjFromStr(content.c_str()));
        KlassObj_setMember(ko,"filename",ZObjFromStr(filename.c_str()));
        ZDict_emplace(payload,ZObjFromStr(partname.c_str()),ZObjFromKlassObj(ko));
      }
      else
        ZDict_emplace(payload,ZObjFromStr(partname.c_str()),ZObjFromStr(content.c_str()));
    }         
  }
  return payload;
}
#endif
