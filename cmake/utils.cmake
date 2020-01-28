# utils.cmake

##########################################################################
#
# 重新定义当前目标的源文件的__FILE__宏, 不包含绝对路径
#
##########################################################################
function(msf_redefine_file targetname)
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
        # Get source file's current list of compile definitions.
        get_property(defs SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        # Get the relative path of the source file in project directory
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        list(APPEND defs "__FILE__=\"${relpath}\"")
        # Set the updated compile definitions on the source file.
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs}
            )
    endforeach()
endfunction()

##########################################################################
#
# Add more 
#
##########################################################################
function(ragelmaker src_rl outputlist outputdir)
    #Create a custom build step that will call ragel on the provided src_rl file.
    #The output .cpp file will be appended to the variable name passed in outputlist.

    get_filename_component(src_file ${src_rl} NAME_WE)

    set(rl_out ${outputdir}/${src_file}.rl.cc)

    #adding to the list inside a function takes special care, we cannot use list(APPEND...)
    #because the results are local scope only
    set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

    #Warning: The " -S -M -l -C -T0  --error-format=msvc" are added to match existing window invocation
    #we might want something different for mac and linux
    add_custom_command(
        OUTPUT ${rl_out}
        COMMAND cd ${outputdir}
        COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -l -C -G2  --error-format=msvc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
        )
    set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
endfunction(ragelmaker)

##########################################################################
#
# Add protobuf make
#
##########################################################################
function(protobufmaker src_proto outputlist outputdir)
    #Create a custom build step that will call ragel on the provided src_rl file.
    #The output .cpp file will be appended to the variable name passed in outputlist.

    get_filename_component(src_file ${src_proto} NAME_WE)
    get_filename_component(src_path ${src_proto} PATH)

    set(protobuf_out ${outputdir}/${src_path}/${src_file}.pb.cc)

    #adding to the list inside a function takes special care, we cannot use list(APPEND...)
    #because the results are local scope only
    set(${outputlist} ${${outputlist}} ${protobuf_out} PARENT_SCOPE)

    add_custom_command(
        OUTPUT ${protobuf_out}
        COMMAND protoc --cpp_out=${outputdir} -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${src_proto}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_proto}
        )
    set_source_files_properties(${protobuf_out} PROPERTIES GENERATED TRUE)
endfunction(protobufmaker)

##########################################################################
#
# Add settings of one executable
# 必须将add_executable声明放到target_link_libraries前面
# target_link_libraries: add_dependencies
#
##########################################################################
function(msf_add_executable targetname srcs depends libs)
    aux_source_directory (${srcs} DIR_SRCS)
    add_executable(${targetname} ${DIR_SRCS})
    add_dependencies(${targetname} ${depends})
    add_definitions(-Wno-builtin-macro-redefined)
    msf_redefine_file(${targetname})
    target_link_libraries(${targetname} ${libs})
endfunction()


##########################################################################
#
# Add settings of one executable
# 必须将add_executable声明放到target_link_libraries前面
#
##########################################################################
function(add_cplusplus_standard_support)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
    CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

    # 设置C++11的属性
    # set(CMAKE_CXX_STANDARD 17)
    # set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if (COMPILER_SUPPORTS_CXX20)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    elseif (COMPILER_SUPPORTS_CXX17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    elseif (COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif (COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.
                        Please use a different C++ compiler.")
    endif()
endfunction()