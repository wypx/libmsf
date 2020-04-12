#!/bin/sh
# if [ ! -d "build" ]; then
if test -e build;then
    echo "-- Build dir exists; rm -rf build and re-run"
    rm -rf build
fi

cd app/agent/proto && protoc -I=./ --cpp_out=./ *.proto
cd -
mkdir -p build/lib
cd build/lib && cmake ../../lib && make && make install
cd -
mkdir -p build/app
cd build/app && cmake ../../app && make && make install