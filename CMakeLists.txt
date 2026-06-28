cmake_minimum_required(VERSION 3.21)
project(redis_simple LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

find_package(spdlog QUIET)

add_custom_target(
  format
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/format.sh"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Format project C++ sources with clang-format"
  VERBATIM
)

function(redis_simple_collect_sources out_var)
  set(sources)
  foreach(source_dir IN LISTS ARGN)
    file(GLOB_RECURSE dir_sources CONFIGURE_DEPENDS
      "${CMAKE_SOURCE_DIR}/${source_dir}/*.cpp"
      "${CMAKE_SOURCE_DIR}/${source_dir}/*.h"
    )
    list(APPEND sources ${dir_sources})
  endforeach()
  set(${out_var} ${sources} PARENT_SCOPE)
endfunction()

function(redis_simple_exclude_sources source_var)
  foreach(pattern IN LISTS ARGN)
    list(FILTER ${source_var} EXCLUDE REGEX "${pattern}")
  endforeach()
  set(${source_var} ${${source_var}} PARENT_SCOPE)
endfunction()

function(redis_simple_configure_target target)
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
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
  add_executable(${target} ${ARGN})
  redis_simple_configure_target(${target})
endfunction()

function(redis_simple_add_gtest_suite suite_name)
  add_test(
    NAME redis_simple_unit_${suite_name}
    COMMAND redis_simple_tests --gtest_filter=${suite_name}.*
  )
  set_tests_properties(redis_simple_unit_${suite_name} PROPERTIES LABELS unit)
endfunction()

redis_simple_collect_sources(REDIS_SIMPLE_CORE_SOURCES
  connection
  event_loop
  memory
  server
  storage
  tcp
  utils
)
redis_simple_exclude_sources(REDIS_SIMPLE_CORE_SOURCES
  "_test\\.cpp$"
  "/server/main\\.cpp$"
)

redis_simple_collect_sources(REDIS_SIMPLE_CLI_SOURCES cli)
redis_simple_exclude_sources(REDIS_SIMPLE_CLI_SOURCES "_test\\.cpp$")

add_library(redis_simple_core STATIC ${REDIS_SIMPLE_CORE_SOURCES})
redis_simple_configure_target(redis_simple_core)

add_library(redis_simple_cli STATIC ${REDIS_SIMPLE_CLI_SOURCES})
redis_simple_configure_target(redis_simple_cli)
target_link_libraries(redis_simple_cli PRIVATE redis_simple_core)

redis_simple_add_executable(redis_simple "server/main.cpp")
target_link_libraries(redis_simple PRIVATE redis_simple_core)

add_subdirectory("third_party/googletest")
enable_testing()

redis_simple_collect_sources(REDIS_SIMPLE_UNIT_TEST_SOURCES
  cli
  memory
  server/reply
  server/reply_utils
  storage
  utils
)
list(FILTER REDIS_SIMPLE_UNIT_TEST_SOURCES INCLUDE REGEX "_test\\.cpp$")

redis_simple_add_executable(redis_simple_tests ${REDIS_SIMPLE_UNIT_TEST_SOURCES})
target_link_libraries(
  redis_simple_tests
  PRIVATE
    redis_simple_cli
    redis_simple_core
    GTest::gtest_main
)

redis_simple_add_gtest_suite(StringUtilsTest)
redis_simple_add_gtest_suite(DictStrTest)
redis_simple_add_gtest_suite(DictIntTest)
redis_simple_add_gtest_suite(DynamicBufferTest)
redis_simple_add_gtest_suite(IntSetTest)
redis_simple_add_gtest_suite(ListPackTest)
redis_simple_add_gtest_suite(QuickListTest)
redis_simple_add_gtest_suite(ReplyBufferTest)
redis_simple_add_gtest_suite(SkiplistTest)
redis_simple_add_gtest_suite(CustomSkiplistTest)
redis_simple_add_gtest_suite(ReplyTest)
redis_simple_add_gtest_suite(TestReplyUtils)
redis_simple_add_gtest_suite(ListTest)
redis_simple_add_gtest_suite(ListEncodingTest)
redis_simple_add_gtest_suite(SetTest)
redis_simple_add_gtest_suite(ZSetListPackTest)
redis_simple_add_gtest_suite(ZSetSkiplistTest)
redis_simple_add_gtest_suite(ZSetSkiplistStandaloneTest)
redis_simple_add_gtest_suite(ZSetTest)
redis_simple_add_gtest_suite(ZSetEncodingTest)
redis_simple_add_gtest_suite(RespParserTest)
redis_simple_add_gtest_suite(ReplyParserTest)
redis_simple_add_gtest_suite(ReplyParseTest)

redis_simple_add_executable(mock_tcp_server "integration/tcp/server_test.cpp")
target_link_libraries(mock_tcp_server PRIVATE redis_simple_core)

redis_simple_add_executable(mock_tcp_client "integration/tcp/client_test.cpp")
target_link_libraries(mock_tcp_client PRIVATE redis_simple_core)

redis_simple_add_executable(
  mock_command_server
  "integration/commands/server_test.cpp"
)
target_link_libraries(mock_command_server PRIVATE redis_simple_core)

redis_simple_add_executable(
  mock_string_client
  "integration/commands/string/client_test.cpp"
)
target_link_libraries(mock_string_client PRIVATE redis_simple_cli)

redis_simple_add_executable(
  mock_set_client
  "integration/commands/set/client_test.cpp"
)
target_link_libraries(mock_set_client PRIVATE redis_simple_cli)

redis_simple_add_executable(
  mock_list_client
  "integration/commands/list/client_test.cpp"
)
target_link_libraries(mock_list_client PRIVATE redis_simple_cli)

redis_simple_add_executable(
  mock_zset_client
  "integration/commands/zset/client_test.cpp"
)
target_link_libraries(mock_zset_client PRIVATE redis_simple_cli)

add_test(
  NAME redis_simple_integration_tcp
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/scripts/run_integration_tcp.sh"
    $<TARGET_FILE:mock_tcp_server>
    $<TARGET_FILE:mock_tcp_client>
)

add_test(
  NAME redis_simple_integration_command_string
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/scripts/run_integration_commands.sh"
    $<TARGET_FILE:mock_command_server>
    $<TARGET_FILE:mock_string_client>
)

add_test(
  NAME redis_simple_integration_command_set
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/scripts/run_integration_commands.sh"
    $<TARGET_FILE:mock_command_server>
    $<TARGET_FILE:mock_set_client>
)

add_test(
  NAME redis_simple_integration_command_list
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/scripts/run_integration_commands.sh"
    $<TARGET_FILE:mock_command_server>
    $<TARGET_FILE:mock_list_client>
)

add_test(
  NAME redis_simple_integration_command_zset
  COMMAND
    /bin/sh
    "${CMAKE_SOURCE_DIR}/scripts/run_integration_commands.sh"
    $<TARGET_FILE:mock_command_server>
    $<TARGET_FILE:mock_zset_client>
)

set_tests_properties(
  redis_simple_integration_tcp
  redis_simple_integration_command_string
  redis_simple_integration_command_set
  redis_simple_integration_command_list
  redis_simple_integration_command_zset
  PROPERTIES
    LABELS integration
    RESOURCE_LOCK redis_simple_port_8080
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's tests" FORCE)
add_subdirectory("third_party/benchmark")

redis_simple_collect_sources(REDIS_SIMPLE_BENCHMARK_SOURCES benchmarks)

redis_simple_add_executable(memory_benchmark ${REDIS_SIMPLE_BENCHMARK_SOURCES})
target_link_libraries(
  memory_benchmark
  PRIVATE
    redis_simple_core
    benchmark::benchmark
)
