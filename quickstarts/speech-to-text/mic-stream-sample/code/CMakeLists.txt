cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_USE_SYSTEM_ENVIRONMENT_PATH ON)

project(speech)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

get_filename_component(proto "../protos/yysystem.proto" ABSOLUTE)
get_filename_component(proto_path "${proto}" DIRECTORY)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_NLOHMANN_JSON_NLOHMANN_JSON nlohmann_json::nlohmann_json)
# message(STATUS "_PROTOBUF_LIBPROTOBUF: ${_PROTOBUF_LIBPROTOBUF}")
# message(STATUS "_REFLECTION: ${_REFLECTION}")
# message(STATUS "_NLOHMANN_JSON_NLOHMANN_JSON: ${_NLOHMANN_JSON_NLOHMANN_JSON}")

find_package(portaudio CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# message(STATUS "Using gRPC ${gRPC_VERSION}")
# message(STATUS "Using protobuf ${Protobuf_VERSION}")
# message(STATUS "Using nlohmann_json ${nlohmann_json_VERSION}")
# message(STATUS "Using Threads ${threads_VERSION}")
message(STATUS "Using portaudio ${portaudio_VERSION}")
message(STATUS "PORTAUDIO_INCLUDE_DIRS: ${PORTAUDIO_INCLUDE_DIRS}")
message(STATUS "PORTAUDIO_LIBRARIES: ${PORTAUDIO_LIBRARIES}")

find_program(_PROTOBUF_PROTOC protoc)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
# message(STATUS "_PROTOBUF_PROTOC: ${_PROTOBUF_PROTOC}")
# message(STATUS "_GRPC_CPP_PLUGIN_EXECUTABLE: ${_GRPC_CPP_PLUGIN_EXECUTABLE}")

set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/yysystem.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/yysystem.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/yysystem.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/yysystem.grpc.pb.h")
add_custom_command(
  OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${proto}"
  DEPENDS "${proto}"
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(grpc_proto
  ${grpc_srcs}
  ${grpc_hdrs}
  ${proto_srcs}
  ${proto_hdrs}
)
target_link_libraries(grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

add_library(config STATIC config.cc config.h)
target_link_libraries(config PUBLIC
  grpc_proto
  ${_NLOHMANN_JSON_NLOHMANN_JSON}
)

add_executable(${PROJECT_NAME} speech.cc)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    portaudio_static
    config
)