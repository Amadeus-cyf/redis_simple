cmake_minimum_required(VERSION 3.9)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(redis_simple "src/server.cpp")
target_sources(redis_simple
  PRIVATE
    "src/client.h"
    "src/client.cpp"
    "src/server.h"
    "src/server.cpp"
    "src/expire.h"
    "src/expire.cpp"
    "src/t_string.h"
    "src/t_string.cpp"
    "src/db/db.h"
    "src/db/db.cpp"
    "src/db/redis_obj.h"
    "src/conn_handler/conn_handler.h"
    "src/conn_handler/conn_handler.cpp"
    "src/connection/connection.h"
    "src/connection/connection.cpp"
    "src/connection/tcp.h"
    "src/connection/tcp.cpp"
    "src/event_loop/ae.h"
    "src/event_loop/ae.cpp"
    "src/event_loop/ae_file_event.h"
    "src/event_loop/ae_kqueue.h"
    "src/event_loop/ae_kqueue.cpp"
    "src/event_loop/ae_time_event.h"
    "src/memory/buf_node.h"
    "src/memory/dict.h"
    "src/memory/query_buffer.h"
    "src/memory/query_buffer.cpp"
    "src/memory/reply_buffer.h"
    "src/memory/reply_buffer.cpp"
    "src/networking/networking.h"
    "src/networking/networking.cpp"
    "src/networking/handler/read_client.h"
    "src/networking/handler/read_client.cpp"
    "src/networking/handler/write_client.h"
    "src/networking/handler/write_client.cpp"
    "src/redis_cmd/redis_cmd.h"
    "src/redis_cmd/redis_cmd.cpp"
    "src/utils/string_utils.h"
    "src/utils/string_utils.cpp"
    "src/reply/reply.h"
    "src/reply/reply.cpp"
)

add_subdirectory("third_party/googletest")

add_executable(memory_test "") 
target_sources(memory_test
  PRIVATE
    "src/utils/string_utils.h"
    "src/utils/string_utils.cpp"
    "src/memory/buf_node.h"
    "src/memory/query_buffer.h"
    "src/memory/query_buffer.cpp"
    "src/memory/reply_buffer.h"
    "src/memory/reply_buffer.cpp"
    "test/memory/dict_test.cpp"
    "test/memory/query_buffer_test.cpp"
    "test/memory/reply_buffer_test.cpp"
)

add_executable(reply_test "")
target_sources(reply_test
  PRIVATE
    "src/reply/reply.h"
    "src/reply/reply.cpp"
    "test/reply/reply_test.cpp"
)

target_link_libraries(
  memory_test
  GTest::gtest_main
)

target_link_libraries(
  reply_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  memory_test
)

add_subdirectory("third_party/benchmark")

add_executable(memory_benchmark "")
target_sources(memory_benchmark
  PRIVATE
    "src/utils/string_utils.h"
    "src/utils/string_utils.cpp"
    "src/memory/buf_node.h"
    "src/memory/query_buffer.h"
    "src/memory/query_buffer.cpp"
    "src/memory/reply_buffer.h"
    "src/memory/reply_buffer.cpp"
    "benchmarks/buffer.h"
    "benchmarks/dict_benchmark.cpp"
    "benchmarks/query_buffer_benchmark.cpp"
    "benchmarks/reply_buffer_benchmark.cpp"
)

target_link_libraries(
  memory_benchmark
  benchmark::benchmark
)
