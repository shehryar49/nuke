#ifndef REQUEST_H_
#define REQUEST_H_
#include "nuke.h"
#include <fcgiapp.h>
Klass* reqKlass;
extern PltObject nil;
extern "C"
{
    //Request methods
    PltObject GetParam(PltObject*,int32_t);
}

//Implementations
PltObject GetParam(PltObject* args,int32_t n)
{
    if(n!=2)
      return Plt_Err(ArgumentError,"Error GetParam() takes 1 argument!");
    if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr) ->klass != reqKlass)
      return Plt_Err(TypeError,"Error self must be an object of request class!");
    if(args[1].type!=PLT_STR)
      return Plt_Err(TypeError,"Parameter name must be a string!");
    KlassObject& self = AS_KlASSOBJECT(args[0]);
    FCGX_Request* req = (FCGX_Request*)self.members[".ptr"].ptr; 
    string& name = AS_STR(args[1]); 
    char* val = FCGX_GetParam(name.c_str(),req->envp);
    if(!val)
      return nil;
    return PObjFromStr((string)val);
}
#endif
