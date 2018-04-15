cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
project(zero_preprocessor)

# Prepare "Catch" library for other executables TODO move catch inside project?
set(CATCH_INCLUDE_DIR /usr/local/include/catch/)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

message("cur source dir " ${CMAKE_CURRENT_SOURCE_DIR})
message("cur project dir " ${PROJECT_SOURCE_DIR})
# Make test executable
set(TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tests_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_std_rules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_std_parser.cpp)
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
find_package(Boost)
target_link_libraries(tests PRIVATE Catch Boost::boost)

function(preprocess target)
  get_target_property(sources ${target} SOURCES)
  foreach(src IN LISTS sources)
    # TODO: throw exception for absolute path or relative path to a lower directory
    # OR maybe just message to the user and continue?
    add_custom_command(
      OUTPUT ${src}
      COMMAND main ${CMAKE_SOURCE_DIR}/${src} ${CMAKE_CURRENT_BINARY_DIR}/${src}
      DEPENDS ${CMAKE_SOURCE_DIR}/${src} main
      )
  endforeach()
endfunction()

# Our main
add_executable(main ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_compile_features(main INTERFACE cxx_std_17)
target_include_directories(main PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE Boost::boost -lstdc++fs)
