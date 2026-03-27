cmake_minimum_required(VERSION 3.14)
project(diff_lib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 库目标 ----
add_library(diff_lib STATIC
    src/diff_value.cpp
    src/async_writer.cpp
    src/diff_client.cpp
)

target_include_directories(diff_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(diff_lib PRIVATE -Wall -Wextra -O2)

# ---- 可选：Protobuf 集成（取消注释后启用） ----
# find_package(Protobuf REQUIRED)
# target_link_libraries(diff_lib PUBLIC protobuf::libprotobuf)
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/diff_message.proto)
# target_sources(diff_lib PRIVATE ${PROTO_SRCS})
# target_include_directories(diff_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# ---- 单元测试 ----
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- 安装 ----
install(TARGETS diff_lib ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/diff_lib)
