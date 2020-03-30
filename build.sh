#!/bin/sh
# if [ ! -d "build" ]; then
if test -e build;then
    echo "-- Build dir exists; rm -rf build and re-run"
    rm -rf build
fi

mkdir -p build/lib
cd build/lib && cmake ../../lib && make && make install
mkdir -p build/app
cd build/app && cmake ../../app && make && make install