cmake_minimum_required(VERSION 3.0.0)

project(reflected-project)

add_executable(example src/main.cpp)

# add our preprocessor
set(preprocessor_dir "./../../")
set(preprocessor_build_dir "./preprocessor_build")
add_subdirectory(${preprocessor_dir} ${preprocessor_build_dir} EXCLUDE_FROM_ALL)
# preprocess our example target
preprocess(example)
