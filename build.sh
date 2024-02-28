#!/usr/bin/sh
g++ -shared src/nuke.cpp -o nuke.so -fPIC -lfcgi -lpthread -DNDEBUG -O3
sudo cp nuke.so /opt/zuko/modules
