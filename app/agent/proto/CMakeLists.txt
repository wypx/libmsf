aux_source_directory(. DIR_SRCS)
add_library(msf_proto STATIC ${DIR_SRCS})

msf_redefine_file(msf_proto)
add_definitions(-Wno-builtin-macro-redefined)

install(TARGETS msf_proto LIBRARY DESTINATION lib)       # 安装到 /home/luotang.me/lib
#install(FILES * DESTINATION include/msf/base/)          # 安装到 usr/include/msf/base

