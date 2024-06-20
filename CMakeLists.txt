cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)
project(nuke)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(UNIX)
  link_directories(/opt/zuko/lib)
else()
  link_directories(C:/zuko/lib)
endif()

add_library(nuke SHARED src/nuke.cpp src/app.cpp src/response.cpp src/request.cpp src/utils.cpp)
target_link_libraries(nuke -lfcgi)
if(UNIX)
  target_link_libraries(nuke -lpthread)
endif()

target_link_libraries(nuke -lzapi)

