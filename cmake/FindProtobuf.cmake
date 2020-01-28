# https://cmake.org/cmake/help/v3.6/module/FindProtobuf.html

find_package(Protobuf)
if(Protobuf_FOUND)
    MESSAGE(STATUS "INFO: Protobuf library is found.")
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()