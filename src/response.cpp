#include "response.h"

zclass* res_class;

zobject RES_CONSTRUCT(zobject* args,int32_t n)
{
  int statusCode = 200;
  if(n==3)
  ;
  else if(n == 4)
  {
    if(args[3].type != Z_INT)
      return z_err(TypeError,"Status code must be an integer!");
    statusCode = args[3].i;
    if(statusCode <= 0)
      return z_err(ValueError,"Error status code must be greater than 0");
  }
  else
    return z_err(ArgumentError,"Either 3 or 4 arguments required!");
  if(args[0].type != Z_OBJ || ((zclass_object*)args[0].ptr)->_klass != res_class)
    return z_err(TypeError,"'self' must be an object of response class.");
  if(args[1].type!=Z_STR)
    return z_err(TypeError,"Argument 2 must be string!");
  if(args[2].type!=Z_STR && args[2].type!=Z_BYTEARR)
    return z_err(TypeError,"Argument 3 must be either a string or a bytearray!");
  
  zclass_object* self = (zclass_object*)args[0].ptr;

  zclassobj_set(self,".type",args[1]);
  zclassobj_set(self,".content",args[2]);
  zclassobj_set(self,".status",zobj_from_int(statusCode));
  return zobj_nil();
}
