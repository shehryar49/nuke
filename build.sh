#!/usr/bin/sh
g++ -shared src/nuke.cpp -o nuke.so -fPIC -lfcgi -lpthread -lzapi -DNDEBUG -O3 -L /opt/zuko/lib
sudo cp nuke.so /opt/zuko/modules
