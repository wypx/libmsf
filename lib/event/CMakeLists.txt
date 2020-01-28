aux_source_directory(. DIR_SRCS)
add_library(msf_event STATIC ${DIR_SRCS})
msf_redefine_file(msf_event)
add_definitions(-Wno-builtin-macro-redefined)

#install(TARGETS lib LIBRARY DESTINATION lib)            # 安装到 usr/lib
#install(FILES * DESTINATION include/msf/net/)          # 安装到 usr/include/msf/base