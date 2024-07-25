cmake_minimum_required(VERSION 3.9)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

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
  "server/commands/command.h"
  "server/commands/command.cpp"
  "server/commands/t_set/sadd.h"
  "server/commands/t_set/sadd.cpp"
  "server/commands/t_set/scard.h"
  "server/commands/t_set/scard.cpp"
  "server/commands/t_set/sismember.h"
  "server/commands/t_set/sismember.cpp"
  "server/commands/t_set/smembers.h"
  "server/commands/t_set/smembers.cpp"
  "server/commands/t_set/srem.h"
  "server/commands/t_set/srem.cpp"
  "server/commands/t_string/get.h"
  "server/commands/t_string/get.cpp"
  "server/commands/t_string/set.h"
  "server/commands/t_string/set.cpp"
  "server/commands/t_string/delete.h"
  "server/commands/t_string/delete.cpp"
  "server/commands/t_zset/zadd.h"
  "server/commands/t_zset/zadd.cpp"
  "server/commands/t_zset/zcard.h"
  "server/commands/t_zset/zcard.cpp"
  "server/commands/t_zset/zrank.h"
  "server/commands/t_zset/zrank.cpp"
  "server/commands/t_zset/zrange.h"
  "server/commands/t_zset/zrange.cpp"
  "server/commands/t_zset/zrem.h"
  "server/commands/t_zset/zrem.cpp"
  "server/commands/t_zset/zscore.h"
  "server/commands/t_zset/zscore.cpp"
  "server/db/db.h"
  "server/db/db.cpp"
  "server/db/redis_obj.h"
  "server/db/redis_obj.cpp"
  "server/networking/networking.h"
  "server/networking/networking.cpp"
  "server/networking/redis_cmd.h"
  "server/networking/redis_cmd.cpp"
  "server/networking/conn_handler/conn_handler.h"
  "server/networking/conn_handler/conn_handler.cpp"
  "server/networking/conn_handler/read_client.h"
  "server/networking/conn_handler/read_client.cpp"
  "server/networking/conn_handler/write_client.h"
  "server/networking/conn_handler/write_client.cpp"
  "server/reply/reply.h"
  "server/reply/reply.cpp"
  "server/reply_utils/reply_utils.h"
  "server/set/set.h"
  "server/set/set.cpp"
  "server/zset/zset.h"
  "server/zset/zset.cpp"
  "connection/conn_handler.h"
  "connection/connection.h"
  "connection/connection.cpp"
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
  "memory/intset.h"
  "memory/intset.cpp"
  "memory/reply_buffer.h"
  "memory/reply_buffer.cpp"
  "memory/skiplist.h"
  "tcp/tcp.h"
  "tcp/tcp.cpp"
  "event_loop/ae.h"
  "event_loop/ae.cpp"
  "utils/string_utils.h"
  "utils/string_utils.cpp"
)
endfunction()

redis_simple_add_source(redis_simple "server/main.cpp")

add_subdirectory("third_party/googletest")

add_executable(redis_simple_tests "") 
target_sources(redis_simple_tests
  PRIVATE
    "utils/string_utils.h"
    "utils/string_utils.cpp"
    "utils/string_utils_test.cpp"
    "memory/buf_node.h"
    "memory/dict.h"
    "memory/dict_test.cpp"
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "memory/dynamic_buffer_test.cpp"
    "memory/intset.h"
    "memory/intset.cpp"
    "memory/intset_test.cpp"
    "memory/reply_buffer.h"
    "memory/reply_buffer.cpp"
    "memory/reply_buffer_test.cpp"
    "memory/skiplist.h"
    "memory/skiplist_test.cpp"
    "server/reply/reply.h"
    "server/reply/reply.cpp"
    "server/reply/reply_test.cpp"
    "server/reply_utils/reply_utils.h"
    "server/reply_utils/reply_utils_test.cpp"
    "server/set/set.h"
    "server/set/set.cpp"
    "server/set/set_test.cpp"
    "server/zset/zset.h"
    "server/zset/zset.cpp"
    "server/zset/zset_test.cpp"
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

function(redis_simple_cli_add_source source source_file)
add_executable(${source} "")
target_sources(${source}
  PRIVATE
  ${source_file}
    "cli/cli.h"
    "cli/cli.cpp"
    "cli/completable_future.h"
    "cli/resp_parser.h"
    "cli/resp_parser.cpp"
    "connection/conn_handler.h"
    "connection/connection.h"
    "connection/connection.cpp"
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
    "memory/dynamic_buffer.h"
    "memory/dynamic_buffer.cpp"
    "utils/string_utils.h"
    "utils/string_utils.cpp"
)
endfunction()

redis_simple_add_source(mock_command_server "test/command/server_test.cpp")
redis_simple_cli_add_source(mock_t_string_client "test/command/t_string/client_test.cpp")
redis_simple_cli_add_source(mock_t_set_client "test/command/t_set/client_test.cpp")
redis_simple_cli_add_source(mock_t_zset_client "test/command/t_zset/client_test.cpp")

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
