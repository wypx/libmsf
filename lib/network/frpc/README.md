frpc
=======

frpc is a small C++ RPC library built on top of libmsf and google's
protocol buffer, sponsored by [Springbeats](http://springbeats.com), and
released under GNU License.

Goals
-----

frpc aims to provide a simple Remote Procedure Call API for C++ programs.
It was initially built to simplify client/server communications in desktop
applications that were already using protobuf as a payload format.

Because it is built on top of Boost, it is quite portable.

Getting Started
---------------

### Dependencies

* [libmsf](https://github.com/wypx/libmsf)
* [Google Protobuf](https://github.com/protocolbuffers/protobuf.git)
* [Google Gtest](https://github.com/google/googletest.git) which is already included in protobuf's archive.

Note that the license for all these components allow a commercial usage (please
have a look at them for details).

### Building

Build system relies on [cmake](https://cmake.org/)

For example using cmake:

    ./build.py

This will build the static lib and unit-tests in the bin/ directory.

### Defining your service in protobuf

    // File: echo.proto
    package echo;
    option cc_generic_services = true;

    message EchoRequest
    {
      required string message = 1;
    }

    message EchoResponse
    {
      required string response = 1;
    }

    service EchoService
    {
      rpc Echo(EchoRequest) returns (EchoResponse);
    }

Compilation with `protoc --cpp_out=. echo.proto` generates echo.pb.h/echo.pb.cc files.

### Using your service in Server code

TODO

### Using your service in Client code

TODO
