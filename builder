#!/bin/bash

if [ ! -d "build" ]
then 
    mkdir build
fi

# build client 
gcc client/main.c -o build/client

# build server
gcc server/main.c -o build/server

