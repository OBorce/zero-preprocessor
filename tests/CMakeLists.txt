cmake_minimum_required(VERSION 3.13)

find_package(Catch2 REQUIRED)

# Make test executable
set(TEST_SOURCES
  tests_main.cpp
  test_std_rules.cpp
  test_meta_classes_rules.cpp
  test_std_parser.cpp
  )
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE
  ${zero_preprocessor_SOURCE_DIR}/include
  ${zero_preprocessor_SOURCE_DIR}/extern/static_reflection
  ${zero_preprocessor_SOURCE_DIR}/extern/meta_classes/
  )

target_compile_options(tests PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-ftemplate-backtrace-limit=0>
  $<$<CXX_COMPILER_ID:Clang>:-ftemplate-backtrace-limit=0>
  )

target_compile_options(tests PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-fsanitize=address -fno-omit-frame-pointer>
  $<$<CXX_COMPILER_ID:Clang>:-fsanitize=address -fno-omit-frame-pointer>
  )

target_link_options(tests PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-fsanitize=address>
  $<$<CXX_COMPILER_ID:Clang>:-fsanitize=address>
  )

target_link_libraries(tests PRIVATE Catch2::Catch2 Boost::boost)

add_test(NAME test COMMAND tests)
