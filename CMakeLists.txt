cmake_minimum_required(VERSION 3.21)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(spdlog QUIET)

function(redis_simple_enable_logging target)
  target_compile_options(
    ${target}
    PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-include>
      $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/logging/logger.h>
  )
  if(TARGET spdlog::spdlog)
    target_compile_definitions(${target} PRIVATE REDIS_SIMPLE_USE_SPDLOG)
    target_link_libraries(${target} PRIVATE spdlog::spdlog)
  endif()
endfunction()

function(redis_simple_add_executable target)
  add_executable(${target} "")
  target_sources(${target} PRIVATE ${ARGN})
  redis_simple_enable_logging(${target})
endfunction()

set(REDIS_SIMPLE_UTILS
  "utils/float_utils.h"
  "utils/float_utils.cpp"
  "utils/int_utils.h"
  "utils/int_utils.cpp"
  "utils/string_utils.h"
  "utils/string_utils.cpp"
)

set(REDIS_SIMPLE_MEMORY
  "memory/buf_node.h"
  "memory/dict.h"
  "memory/dynamic_buffer.h"
  "memory/dynamic_buffer.cpp"
  "memory/intset.h"
  "memory/intset.cpp"
  "memory/listpack.h"
  "memory/listpack.cpp"
  "memory/reply_buffer.h"
  "memory/reply_buffer.cpp"
  "memory/skiplist.h"
)

set(REDIS_SIMPLE_EVENT_LOOP
  "event_loop/ae.h"
  "event_loop/ae.cpp"
  "event_loop/ae_file_event.h"
  "event_loop/ae_file_event_impl.h"
  "event_loop/ae_kqueue.h"
  "event_loop/ae_kqueue.cpp"
  "event_loop/ae_time_event.h"
  "event_loop/ae_time_event_impl.h"
)

set(REDIS_SIMPLE_TCP
  "tcp/tcp.h"
  "tcp/tcp.cpp"
)

set(REDIS_SIMPLE_CONNECTION
  "connection/conn_handler.h"
  "connection/connection.h"
  "connection/connection.cpp"
)

set(REDIS_SIMPLE_STORAGE
  "storage/list/list.h"
  "storage/list/list.cpp"
  "storage/set/set.h"
  "storage/set/set.cpp"
  "storage/zset/zset_entry.h"
  "storage/zset/zset_range_spec.h"
  "storage/zset/zset_storage.h"
  "storage/zset/zset_listpack.h"
  "storage/zset/zset_listpack.cpp"
  "storage/zset/zset_skiplist.h"
  "storage/zset/zset_skiplist.cpp"
  "storage/zset/zset.h"
  "storage/zset/zset.cpp"
)

set(REDIS_SIMPLE_SERVER
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
)

set(REDIS_SIMPLE_CORE
  ${REDIS_SIMPLE_SERVER}
  ${REDIS_SIMPLE_STORAGE}
  ${REDIS_SIMPLE_CONNECTION}
  ${REDIS_SIMPLE_EVENT_LOOP}
  ${REDIS_SIMPLE_MEMORY}
  ${REDIS_SIMPLE_TCP}
  ${REDIS_SIMPLE_UTILS}
)

set(REDIS_SIMPLE_CLI
  "cli/cli.h"
  "cli/cli.cpp"
  "cli/completable_future.h"
  "cli/resp_parser.h"
  "cli/resp_parser.cpp"
  ${REDIS_SIMPLE_CONNECTION}
  ${REDIS_SIMPLE_EVENT_LOOP}
  ${REDIS_SIMPLE_TCP}
  "memory/dynamic_buffer.h"
  "memory/dynamic_buffer.cpp"
  ${REDIS_SIMPLE_UTILS}
)

redis_simple_add_executable(redis_simple "server/main.cpp" ${REDIS_SIMPLE_CORE})

add_subdirectory("third_party/googletest")
enable_testing()

redis_simple_add_executable(
  redis_simple_tests
  ${REDIS_SIMPLE_UTILS}
  "utils/string_utils_test.cpp"
  ${REDIS_SIMPLE_MEMORY}
  "memory/dict_test.cpp"
  "memory/dynamic_buffer_test.cpp"
  "memory/intset_test.cpp"
  "memory/listpack_test.cpp"
  "memory/reply_buffer_test.cpp"
  "memory/skiplist_test.cpp"
  "server/reply/reply.h"
  "server/reply/reply.cpp"
  "server/reply/reply_test.cpp"
  "server/reply_utils/reply_utils.h"
  "server/reply_utils/reply_utils_test.cpp"
  ${REDIS_SIMPLE_STORAGE}
  "storage/list/list_test.cpp"
  "storage/set/set_test.cpp"
  "storage/zset/zset_listpack_test.cpp"
  "storage/zset/zset_skiplist_test.cpp"
  "storage/zset/zset_test.cpp"
  "cli/resp_parser.h"
  "cli/resp_parser.cpp"
  "cli/resp_parser_test.cpp"
)

target_link_libraries(redis_simple_tests GTest::gtest_main)
add_test(NAME redis_simple_tests COMMAND redis_simple_tests)

redis_simple_add_executable(
  mock_tcp_server
  "integration/tcp/server_test.cpp"
  ${REDIS_SIMPLE_TCP}
)

redis_simple_add_executable(
  mock_tcp_client
  "integration/tcp/client_test.cpp"
  ${REDIS_SIMPLE_TCP}
)

redis_simple_add_executable(
  mock_event_loop_server
  "integration/event_loop/server_test.cpp"
  ${REDIS_SIMPLE_EVENT_LOOP}
  ${REDIS_SIMPLE_TCP}
)

redis_simple_add_executable(
  mock_event_loop_client
  "integration/event_loop/client_test.cpp"
  ${REDIS_SIMPLE_EVENT_LOOP}
  ${REDIS_SIMPLE_TCP}
)

redis_simple_add_executable(
  mock_server
  "integration/server/server_test.cpp"
  ${REDIS_SIMPLE_CORE}
)

redis_simple_add_executable(
  mock_client
  "integration/server/client_test.cpp"
  ${REDIS_SIMPLE_CORE}
)

redis_simple_add_executable(
  mock_command_server
  "integration/command/server_test.cpp"
  ${REDIS_SIMPLE_CORE}
)

redis_simple_add_executable(
  mock_t_string_client
  "integration/command/t_string/client_test.cpp"
  ${REDIS_SIMPLE_CLI}
)

redis_simple_add_executable(
  mock_t_set_client
  "integration/command/t_set/client_test.cpp"
  ${REDIS_SIMPLE_CLI}
)

redis_simple_add_executable(
  mock_t_zset_client
  "integration/command/t_zset/client_test.cpp"
  ${REDIS_SIMPLE_CLI}
)

add_test(
  NAME redis_simple_integration_tcp
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/integration/run_tcp_test.sh"
    $<TARGET_FILE:mock_tcp_server>
    $<TARGET_FILE:mock_tcp_client>
)

add_test(
  NAME redis_simple_integration_commands
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/integration/run_command_tests.sh"
    $<TARGET_FILE:mock_command_server>
    $<TARGET_FILE:mock_t_string_client>
    $<TARGET_FILE:mock_t_set_client>
    $<TARGET_FILE:mock_t_zset_client>
)

set_tests_properties(
  redis_simple_integration_tcp
  redis_simple_integration_commands
  PROPERTIES
    RESOURCE_LOCK redis_simple_port_8080
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's tests" FORCE)
add_subdirectory("third_party/benchmark")

redis_simple_add_executable(
  memory_benchmark
  ${REDIS_SIMPLE_UTILS}
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

target_link_libraries(memory_benchmark benchmark::benchmark)
