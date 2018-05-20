cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
project(meta-project)

add_executable(example src/main.cpp)
target_compile_features(example INTERFACE cxx_std_17)
target_include_directories(example PUBLIC ${PROJECT_SOURCE_DIR})

# add our preprocessor
set(preprocessor_dir "./../../")
set(preprocessor_build_dir "./preprocessor_build")
add_subdirectory(${preprocessor_dir} ${preprocessor_build_dir} EXCLUDE_FROM_ALL)
# preprocess our example target
preprocess(example ${preprocessor_dir})
