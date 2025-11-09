#!/bin/sh -ef

cd "$(dirname "$0")"/..

if [ ! -d "build" ]
then
     mkdir "build"
fi

cd build

cmake .. ${@:+"$@"}
cmake --build .
