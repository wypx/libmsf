# c++17
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC -O3 -Wall -g -ggdb")

add_subdirectory (Proto AgentProto)
add_subdirectory (Client AgentClient)
add_subdirectory (Server AgentServer)