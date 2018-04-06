cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
project(zero_preprocessor)

# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR /usr/local/include/catch/)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

set(BOOST_INCLUDE_DIR /usr/local/include/boost_1_66_0/)

# Make test executable
set(TEST_SOURCES tests_main.cpp test_std_rules.cpp test_std_parser.cpp)
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE ./ ${BOOST_INCLUDE_DIR})
target_link_libraries(tests Catch)
