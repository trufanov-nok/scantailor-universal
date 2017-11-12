#!/bin/bash

cd ../../build
cp ../resources/appicon.svg scantailor-universal.svg
# cmake and make are called here just to make sure that correct
# path prefix will be used in config.h and therefore in binary
# this path is used to search translation files
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
cp scantailor scantailor-universal
cp scantailor-cli scantailor-universal-cli
