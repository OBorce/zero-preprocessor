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
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_std_rules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_meta_classes_rules.cpp
  #${CMAKE_CURRENT_SOURCE_DIR}/tests/test_std_parser.cpp
  )
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/extern/static_reflection
  ${PROJECT_SOURCE_DIR}/extern/meta_classes/
  )

set(Boost_USE_STATIC_LIBS   ON)
find_package(Boost 1.67 REQUIRED COMPONENTS filesystem)
target_link_libraries(tests PRIVATE Catch Boost::boost)
enable_testing()
add_test(NAME test COMMAND tests)

function(preprocess target preprocessor_dir)
  get_target_property(sources ${target} SOURCES)
  get_target_property(includes ${target} INCLUDE_DIRECTORIES)
  message("include directories: ${includes}")
  set(index "0")
  foreach(src IN LISTS sources)
    MATH(EXPR index "${index}+1")

    # TODO: throw exception if src is absolute path or relative path to a lower directory
    # OR maybe just message to the user and continue?

    get_filename_component(src_file_name ${src} NAME)

    # get all the includes for the source
    add_custom_command(
      OUTPUT ${src}_target
      COMMAND main 1
      ${CMAKE_SOURCE_DIR}/${src}
      ${CMAKE_CURRENT_BINARY_DIR}/out/includes${index}.txt
      ${includes}
      BYPRODUCTS includes${index}.txt
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Checking includes for ${src}"
      )

    # generate the meta for all the includes and the source
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/meta_out/${src_file_name}
      COMMAND main 2
      ${CMAKE_SOURCE_DIR}/${src}
      ${CMAKE_CURRENT_BINARY_DIR}/out/includes${index}.txt
      ${CMAKE_CURRENT_BINARY_DIR}/meta_out
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS main ${src}_target
      COMMENT "Generating meta classes for ${src}"
      )

    add_executable(meta ${CMAKE_CURRENT_BINARY_DIR}/meta_out/${src_file_name})
    target_include_directories(meta PRIVATE
      ${preprocessor_dir}/extern/meta_classes/meta_include
      ${CMAKE_CURRENT_BINARY_DIR}/meta_out
      ${preprocessor_dir}/extern/static_reflection/out_include
      )


    # finally preprocess the source
    add_custom_command(
      OUTPUT ${src}
      COMMAND main 3
      ${CMAKE_SOURCE_DIR}/${src}
      ${CMAKE_CURRENT_BINARY_DIR}/${src}
      ${CMAKE_CURRENT_BINARY_DIR}/out/includes${index}.txt
      $<TARGET_FILE:meta>
      DEPENDS ${CMAKE_SOURCE_DIR}/${src} main meta
      )
  endforeach()
  target_include_directories(${target} PRIVATE ${preprocessor_dir}/extern/static_reflection/out_include)
  target_include_directories(${target} BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
endfunction()

find_package(Threads REQUIRED)

# Our main
add_executable(main ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_compile_features(main INTERFACE cxx_std_17)
target_include_directories(main PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/extern/static_reflection
  ${PROJECT_SOURCE_DIR}/extern/meta_classes/
  )
target_link_libraries(main PRIVATE Boost::boost Boost::filesystem Threads::Threads -lstdc++fs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
