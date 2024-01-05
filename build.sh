#!/usr/bin/sh
g++ -shared nuke.cpp -o nuke.so -fPIC -lfcgi -g
sudo cp nuke.so /opt/zuko/modules
