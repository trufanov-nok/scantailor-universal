#!/bin/sh -ef

cd "$(dirname "$0")"/..

cd build

ctest . --rerun-failed --output-on-failure
