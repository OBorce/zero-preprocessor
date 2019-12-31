cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
project(zero_preprocessor)

set(Boost_USE_STATIC_LIBS   ON)
find_package(Boost 1.71 REQUIRED COMPONENTS filesystem)

option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    enable_testing()
    add_subdirectory(tests)
endif()

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
  ${zero_preprocessor_SOURCE_DIR}/include
  ${zero_preprocessor_SOURCE_DIR}/extern/static_reflection
  ${zero_preprocessor_SOURCE_DIR}/extern/meta_classes/
  )
target_link_libraries(main PRIVATE Boost::boost Boost::filesystem Threads::Threads -lstdc++fs)

if(MSVC)
  set_target_properties(main PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
