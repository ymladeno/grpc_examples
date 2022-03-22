cmake_minimum_required(VERSION 3.5.1)

project(Grpc_examples)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 14)

set(INSTALL_CMAKE_PATH home/osboxes/.local/lib/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(grpcexamples)

generate_proto_cpp(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../proto/systemmanagerservice.proto
    RETVAL systemmanagerservice_proto_srcs
)

generate_proto_cpp(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../proto/systemmanagerservice_signals.proto
    RETVAL systemmanagerservice_signals_proto_srcs
)

generate_proto_cpp(
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../proto/systemmanagerservice_types.proto
    RETVAL systemmanagerservice_types_proto_srcs
)

add_executable(grpcserver
               system_manager_server.cpp
               ${systemmanagerservice_proto_srcs}
               ${systemmanagerservice_signals_proto_srcs}
               ${systemmanagerservice_types_proto_srcs}
)

target_link_libraries(grpcserver
                    ${_REFLECTION}
                    ${_GRPC_GRPCPP}
                    ${_PROTOBUF_LIBPROTOBUF}
)

target_include_directories(grpcserver PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>
)

add_executable(grpcclient
               system_manager_client.cpp
               ${systemmanagerservice_proto_srcs}
               ${systemmanagerservice_signals_proto_srcs}
               ${systemmanagerservice_types_proto_srcs}
)

target_link_libraries(grpcclient
                    ${_REFLECTION}
                    ${_GRPC_GRPCPP}
                    ${_PROTOBUF_LIBPROTOBUF}
)

target_include_directories(grpcclient PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>
)