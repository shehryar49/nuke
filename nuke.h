#ifndef NUKE_H_
#define NUKE_H_

#ifdef _WIN32
  #include "C:\\plutonium\\PltObject.h"
  #define EXPORT __declspec(dllexport)
#else
  #include "/opt/plutonium/PltObject.h"
  #define EXPORT
#endif


extern "C"
{
    EXPORT PltObject init();
}

#endif