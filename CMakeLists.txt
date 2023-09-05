cmake_minimum_required(VERSION 3.9)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR})

function(redis_simple_add_source source source_file)
add_executable(${source} "")
target_sources(${source}
PRIVATE
  ${source_file}
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
  "server/db/redis_obj.cpp"
  "server/zset/z_set.h"
  "server/zset/z_set.cpp"
  "server/conn_handler/conn_handler.h"
  "server/conn_handler/conn_handler.cpp"
  "server/connection/connection.h"
  "server/connection/connection.cpp"
  "event_loop/ae_file_event.h"
  "event_loop/ae_file_event_impl.h"
  "event_loop/ae_kqueue.h"
  "event_loop/ae_kqueue.cpp"
  "event_loop/ae_time_event.h"
  "event_loop/ae_time_event_impl.h"
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
endfunction()

redis_simple_add_source(redis_simple "server/main.cpp")

add_executable(redis_simple_cli "")
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
    "event_loop/ae_time_event_impl.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "utils/string_utils.h"
    "utils/string_utils.cpp"
)

add_subdirectory("third_party/googletest")

add_executable(redis_simple_tests "") 
target_sources(redis_simple_tests
  PRIVATE
    "utils/string_utils.h"
    "utils/string_utils.cpp"
    "memory/buf_node.h"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "memory/reply_buffer.h"
    "memory/reply_buffer.cpp"
    "memory/skiplist.h"
    "memory/dict_test.cpp"
    "memory/dynamic_buffer_test.cpp"
    "memory/reply_buffer_test.cpp"
    "memory/skiplist_test.cpp"
    "server/reply/reply.h"
    "server/reply/reply.cpp"
    "server/reply/reply_test.cpp"
    "cli/resp_parser.h"
    "cli/resp_parser.cpp"
    "cli/resp_parser_test.cpp"
)

target_link_libraries(
  redis_simple_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  redis_simple_tests
)

add_executable(mock_tcp_server "")
target_sources(mock_tcp_server
  PRIVATE
    "test/tcp/server_test.cpp"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
)

add_executable(mock_tcp_client "")
target_sources(mock_tcp_client
  PRIVATE
    "test/tcp/client_test.cpp"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
)

add_executable(mock_event_loop_server "")
target_sources(mock_event_loop_server
  PRIVATE
    "test/event_loop/server_test.cpp"
    "event_loop/ae.h"
    "event_loop/ae.cpp"
    "event_loop/ae_file_event.h"
    "event_loop/ae_file_event_impl.h"
    "event_loop/ae_kqueue.h"
    "event_loop/ae_kqueue.cpp"
    "event_loop/ae_time_event.h"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
)

add_executable(mock_event_loop_client "")
target_sources(mock_event_loop_client
  PRIVATE
    "test/event_loop/client_test.cpp"
    "event_loop/ae.h"
    "event_loop/ae.cpp"
    "event_loop/ae_file_event.h"
    "event_loop/ae_file_event_impl.h"
    "event_loop/ae_kqueue.h"
    "event_loop/ae_kqueue.cpp"
    "event_loop/ae_time_event.h"
    "event_loop/ae_time_event_impl.h"
    "tcp/tcp.h"
    "tcp/tcp.cpp"
)

redis_simple_add_source(mock_server "test/server/server_test.cpp")
redis_simple_add_source(mock_client "test/server/client_test.cpp")

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
