cmake_minimum_required(VERSION 3.0)

# Prepare "Catch" library for other executables TODO move catch inside project?
set(CATCH_INCLUDE_DIR /usr/local/include/catch/)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

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

target_link_libraries(tests PRIVATE Catch Boost::boost)
add_test(NAME test COMMAND tests)
