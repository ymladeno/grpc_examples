# Module path
list(APPEND CMAKE_MODULE_PATH ${INSTALL_CMAKE_PATH}/grpc ${INSTALL_CMAKE_PATH}/protobuf)


# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

function (generate_proto_cpp)
    cmake_parse_arguments(ARGS "PROTO" "INPUT;OUTPUT_PATH" "PROTO_PATHS;RETVAL" ${ARGN})

    set(inputFile ${ARGS_INPUT})
    set(outputPath ${ARGS_OUTPUT_PATH})
    if (NOT outputPath)
        set(outputPath ${CMAKE_CURRENT_BINARY_DIR})
    endif(NOT outputPath)

    if (ARGS_PROTO)
        message("Generate proto c++ files for ${inputFile}")
    else()
        message("Generate proto-grpc c++ files for ${inputFile}.")
    endif()

    set(RETVAL ${ARGS_RETVAL})

    include_directories(SYSTEM "${outputPath}")

    get_filename_component(inputFile "${inputFile}" ABSOLUTE)
    get_filename_component(inputFile_path "${inputFile}" PATH)
    get_filename_component(modelName "${inputFile}" NAME_WE)

    set(cmdlLineProtoPaths --proto_path "${inputFile_path}")
    set(modelPaths ${ARGS_PROTO_PATHS})
    if (modelPaths)
        foreach(mp ${modelPaths})
            #message("  modelPath=${mp}")
            get_filename_component(modelPathAbsolute "${mp}" ABSOLUTE)
            set(cmdlLineProtoPaths ${cmdlLineProtoPaths} --proto_path "${modelPathAbsolute}")
        endforeach()
    endif(modelPaths)

    if (ARGS_PROTO)
        set(outputFiles
            ${outputPath}/${modelName}.pb.cc
            ${outputPath}/${modelName}.pb.h)
    else()
        set(outputFiles
            ${outputPath}/${modelName}.pb.cc
            ${outputPath}/${modelName}.pb.h
            ${outputPath}/${modelName}.grpc.pb.cc
            ${outputPath}/${modelName}.grpc.pb.h)
    endif()

    #message ("ivs_run_protoc_cpp generator output: ${outputFiles}")
    set_source_files_properties(${outputFiles} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-maybe-uninitialized -Wno-pedantic")
    set (${RETVAL} ${outputFiles} PARENT_SCOPE)

    if (ARGS_PROTO)
        add_custom_command(
            OUTPUT ${outputFiles}
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --cpp_out "${outputPath}"
                ${cmdlLineProtoPaths}
                "${inputFile}"
            DEPENDS "${inputFile}")
    else() 
        add_custom_command(
            OUTPUT ${outputFiles}
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${outputPath}"
                --cpp_out "${outputPath}"
                ${cmdlLineProtoPaths}
                --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
                "${inputFile}"
            DEPENDS "${inputFile}")
    endif()

endfunction (generate_proto_cpp)
