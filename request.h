#ifndef REQUEST_H_
#define REQUEST_H_
#include "nuke.h"
#include <fcgiapp.h>
Klass* reqKlass;
extern ZObject nil;
extern "C"
{
    //Request methods
    ZObject GetParam(ZObject*,int32_t);
}

//Implementations
ZObject GetParam(ZObject* args,int32_t n)
{
    if(n!=2)
      return Z_Err(ArgumentError,"Error GetParam() takes 1 argument!");
    if(args[0].type != Z_OBJ || ((KlassObject*)args[0].ptr) ->klass != reqKlass)
      return Z_Err(TypeError,"Error self must be an object of request class!");
    if(args[1].type!=Z_STR)
      return Z_Err(TypeError,"Parameter name must be a string!");
    KlassObject* self = AS_KlASSOBJ(args[0]);
    FCGX_Request* req = (FCGX_Request*)AS_PTR(KlassObj_getMember(self,".ptr"));
    ZStr* name = AS_STR(args[1]); 
    char* val = FCGX_GetParam(name->val,req->envp);
    if(!val)
      return nil;
    return ZObjFromStr(val);
}
#endif
