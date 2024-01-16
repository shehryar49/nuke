#!/usr/bin/sh
g++ -shared src/nuke.cpp -o nuke.so -fPIC -lfcgi -g -lpthread
sudo cp nuke.so /opt/zuko/modules
