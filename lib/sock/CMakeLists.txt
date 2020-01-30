aux_source_directory(. DIR_SRCS)
add_library(msf_sock STATIC ${DIR_SRCS})
msf_redefine_file(msf_sock)
add_definitions(-Wno-builtin-macro-redefined)

install(TARGETS msf_sock LIBRARY DESTINATION lib)
#install(FILES * DESTINATION include/msf/base/)