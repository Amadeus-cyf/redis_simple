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
    "server/t_cmd.h"
    "server/t_cmd.cpp"
    "server/t_string.h"
    "server/t_string.cpp"
    "server/t_zset.h"
    "server/t_zset.cpp"
    "server/db/db.h"
    "server/db/db.cpp"
    "server/db/redis_obj.h"
    "server/zset/z_set.h"
    "server/zset/z_set.cpp"
    "server/conn_handler/conn_handler.h"
    "server/conn_handler/conn_handler.cpp"
    "server/connection/connection.h"
    "server/connection/connection.cpp"
    "event_loop/ae_file_event_base.h"
    "event_loop/ae_file_event.h"
    "event_loop/ae_kqueue.h"
    "event_loop/ae_kqueue.cpp"
    "event_loop/ae_time_event.h"
    "memory/buf_node.h"
    "memory/dict.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "memory/reply_buffer.h"
    "memory/reply_buffer.cpp"
    "memory/skiplist.h"
    "server/networking/networking.h"
    "server/networking/networking.cpp"
    "server/networking/handler/read_client.h"
    "server/networking/handler/read_client.cpp"
    "server/networking/handler/write_client.h"
    "server/networking/handler/write_client.cpp"
    "server/redis_cmd/redis_cmd.h"
    "server/redis_cmd/redis_cmd.cpp"
    "server/reply/reply.h"
    "server/reply/reply.cpp"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
    "event_loop/ae.h"
    "event_loop/ae.cpp"
    "utils/string_utils.h"
    "utils/string_utils.cpp"
)

add_executable(redis_simple_cli "cli/cli.cpp")
target_sources(redis_simple_cli
  PRIVATE
    "cli/cli.h"
    "cli/cli.cpp"
    "cli/completable_future.h"
    "cli/resp_parser.h"
    "cli/resp_parser.cpp"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
    "event_loop/ae.h"
    "event_loop/ae.cpp"
    "event_loop/ae_file_event.h"
    "event_loop/ae_kqueue.h"
    "event_loop/ae_kqueue.cpp"
    "event_loop/ae_time_event.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "utils/string_utils.h"
    "utils/string_utils.cpp"
)

add_subdirectory("third_party/googletest")

add_executable(memory_test "") 
target_sources(memory_test
  PRIVATE
    "utils/string_utils.h"
    "utils/string_utils.cpp"
    "memory/buf_node.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "memory/reply_buffer.h"
    "memory/reply_buffer.cpp"
    "memory/skiplist.h"
    "test/memory/dict_test.cpp"
    "test/memory/dynamic_buffer_test.cpp"
    "test/memory/reply_buffer_test.cpp"
    "test/memory/skiplist_test.cpp"
)

add_executable(reply_test "")
target_sources(reply_test
  PRIVATE
    "server/reply/reply.h"
    "server/reply/reply.cpp"
    "test/reply/reply_test.cpp"
)

add_executable(resp_parser_test "")
target_sources(resp_parser_test
PRIVATE
  "cli/resp_parser.h"
  "cli/resp_parser.cpp"
  "test/cli/resp_parser_test.cpp"
)

target_link_libraries(
  memory_test
  GTest::gtest_main
)

target_link_libraries(
  reply_test
  GTest::gtest_main
)

target_link_libraries(
  resp_parser_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  memory_test
  reply_test
  resp_parser_test
)

add_subdirectory("third_party/benchmark")

add_executable(memory_benchmark "")
target_sources(memory_benchmark
  PRIVATE
    "utils/string_utils.h"
    "utils/string_utils.cpp"
    "memory/buf_node.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "memory/reply_buffer.h"
    "memory/reply_buffer.cpp"
    "benchmarks/buffer.h"
    "benchmarks/dict_benchmark.cpp"
    "benchmarks/dynamic_buffer_benchmark.cpp"
    "benchmarks/reply_buffer_benchmark.cpp"
)

target_link_libraries(
  memory_benchmark
  benchmark::benchmark
)
