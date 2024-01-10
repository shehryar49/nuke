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
  int statusCode = 200;
  if(n==3)
  ;
  else if(n == 4)
  {
    if(args[3].type != Z_INT)
      return Z_Err(TypeError,"Status code must be an integer!");
    statusCode = args[3].i;
    if(statusCode <= 0)
      return Z_Err(ValueError,"Error status code must be greater than 0");
  }
  else
    return Z_Err(ArgumentError,"Either 3 or 4 arguments required!");
  if(args[0].type != Z_OBJ || ((KlassObject*)args[0].ptr)->klass != resKlass)
    return Z_Err(TypeError,"'self' must be an object of response class.");
  if(args[1].type!=Z_STR)
    return Z_Err(TypeError,"Argument 2 must be string!");
  if(args[2].type!=Z_STR && args[2].type!=Z_BYTEARR)
    return Z_Err(TypeError,"Argument 3 must be either a string or a bytearray!");
  
  KlassObject* self = (KlassObject*)args[0].ptr;

  KlassObj_setMember(self,".type",args[1]);
  KlassObj_setMember(self,".content",args[2]);
  KlassObj_setMember(self,".status",ZObjFromInt(statusCode));
  return nil;
}
