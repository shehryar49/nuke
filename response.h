#include "nuke.h"
#include "utils.h"

extern "C"
{
    //Response methods
    EXPORT ZObject RES_CONSTRUCT(ZObject*,int32_t);
}


extern ZObject nil;
Klass* resKlass;
//Implementation
//Response methods

ZObject RES_CONSTRUCT(ZObject* args,int32_t n)
{
  if(n!=3)
    return Z_Err(ArgumentError,"2 arguments required!");
  if(args[0].type != Z_OBJ || ((KlassObject*)args[0].ptr)->klass != resKlass)
    return Z_Err(TypeError,"'self' must be an object of response class.");
  if(args[1].type!=Z_STR || args[2].type!=Z_STR)
    return Z_Err(TypeError,"Arguments must be strings!");
  KlassObject* self = (KlassObject*)args[0].ptr;

  KlassObj_setMember(self,".type",args[1]);
  KlassObj_setMember(self,".content",args[2]);

  return nil;
}
