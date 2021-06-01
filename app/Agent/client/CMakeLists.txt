aux_source_directory(. DIR_SRCS)
add_library(AgentClient STATIC ${DIR_SRCS})

msf_redefine_file(AgentClient)
add_definitions(-Wno-builtin-macro-redefined)

INSTALL(
  TARGETS AgentClient
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library
)

#install(TARGETS AgentClient LIBRARY DESTINATION lib)       # 安装到 /home/luotang.me/lib
#install(FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base

