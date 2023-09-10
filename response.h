#include "nuke.h"
#include "utils.h"

extern "C"
{
    //Response methods
    EXPORT PltObject RES_CONSTRUCT(PltObject*,int32_t);
}


extern PltObject nil;
Klass* resKlass;
//Implementation
//Response methods

PltObject RES_CONSTRUCT(PltObject* args,int32_t n)
{
  if(n!=3)
    return Plt_Err(ArgumentError,"2 arguments required!");
  if(args[0].type != PLT_OBJ || ((KlassObject*)args[0].ptr)->klass != resKlass)
    return Plt_Err(TypeError,"'self' must be an object of response class.");
  if(args[1].type!=PLT_STR || args[2].type!=PLT_STR)
    return Plt_Err(TypeError,"Arguments must be strings!");
  KlassObject* self = (KlassObject*)args[0].ptr;

  self->members.emplace(".type",args[1]);
  self->members.emplace(".content",args[2]);

  return nil;
}
