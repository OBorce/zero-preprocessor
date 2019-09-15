cmake_minimum_required(VERSION 3.0)

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

target_link_libraries(tests PRIVATE Catch2::Catch2 Boost::boost)
add_test(NAME test COMMAND tests)
