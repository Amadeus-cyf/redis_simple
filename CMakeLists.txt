cmake_minimum_required(VERSION 3.9)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(redis_simple "server/server.cpp")
target_sources(redis_simple
  PRIVATE
    "server/client.h"
    "server/client.cpp"
    "server/server.h"
    "server/server.cpp"
    "server/expire.h"
    "server/expire.cpp"
    "server/t_string.h"
    "server/t_string.cpp"
    "server/db/db.h"
    "server/db/db.cpp"
    "server/db/redis_obj.h"
    "server/conn_handler/conn_handler.h"
    "server/conn_handler/conn_handler.cpp"
    "server/connection/connection.h"
    "server/connection/connection.cpp"
    "server/connection/tcp.h"
    "server/connection/tcp.cpp"
    "server/event_loop/ae.h"
    "server/event_loop/ae.cpp"
    "server/event_loop/ae_file_event.h"
    "server/event_loop/ae_kqueue.h"
    "server/event_loop/ae_kqueue.cpp"
    "server/event_loop/ae_time_event.h"
    "server/memory/buf_node.h"
    "server/memory/dict.h"
    "server/memory/query_buffer.h"
    "server/memory/query_buffer.cpp"
    "server/memory/reply_buffer.h"
    "server/memory/reply_buffer.cpp"
    "server/networking/networking.h"
    "server/networking/networking.cpp"
    "server/networking/handler/read_client.h"
    "server/networking/handler/read_client.cpp"
    "server/networking/handler/write_client.h"
    "server/networking/handler/write_client.cpp"
    "server/redis_cmd/redis_cmd.h"
    "server/redis_cmd/redis_cmd.cpp"
    "server/utils/string_utils.h"
    "server/utils/string_utils.cpp"
    "server/reply/reply.h"
    "server/reply/reply.cpp"
)

add_subdirectory("third_party/googletest")

add_executable(memory_test "") 
target_sources(memory_test
  PRIVATE
    "server/utils/string_utils.h"
    "server/utils/string_utils.cpp"
    "server/memory/buf_node.h"
    "server/memory/query_buffer.h"
    "server/memory/query_buffer.cpp"
    "server/memory/reply_buffer.h"
    "server/memory/reply_buffer.cpp"
    "test/memory/dict_test.cpp"
    "test/memory/query_buffer_test.cpp"
    "test/memory/reply_buffer_test.cpp"
)

add_executable(reply_test "")
target_sources(reply_test
  PRIVATE
    "server/reply/reply.h"
    "server/reply/reply.cpp"
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
    "server/utils/string_utils.h"
    "server/utils/string_utils.cpp"
    "server/memory/buf_node.h"
    "server/memory/query_buffer.h"
    "server/memory/query_buffer.cpp"
    "server/memory/reply_buffer.h"
    "server/memory/reply_buffer.cpp"
    "benchmarks/buffer.h"
    "benchmarks/dict_benchmark.cpp"
    "benchmarks/query_buffer_benchmark.cpp"
    "benchmarks/reply_buffer_benchmark.cpp"
)

target_link_libraries(
  memory_benchmark
  benchmark::benchmark
)
