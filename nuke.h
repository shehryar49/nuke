#ifndef COBRA_H_
#define COBRA_H_

#ifdef _WIN32
  #include "C:\\plutonium\\PltObject.h"
  #define EXPORT __declspec(dllexport)
#else
  #include "/opt/plutonium/PltObject.h"
  #define EXPORT
#endif

#include <fcgiapp.h>

extern "C"
{
    EXPORT PltObject init();

    //App methods

    EXPORT PltObject APP_ROUTE(PltObject*,int32_t);
    EXPORT PltObject APP_RUN(PltObject*,int32_t);
    
    //Response methods
    EXPORT PltObject RES_CONSTRUCT(PltObject*,int32_t);
}

#endif