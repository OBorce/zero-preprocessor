#include <fstream>
#include <iostream>
// TODO: update when moved to GCC 8
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <preprocessor.h>
#include <source.h>
#include <static_reflection.h>
#include <std_parser.h>

/**
 * Check if the output file parent dir exists
 * and create it if it doesn't
 */
void check_out_dir(fs::path out) {
  auto dir = out.parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
}

std::string get_input_content(std::string_view in) {
  // Try to preprocess a file
  std::ifstream in_file(in.data());

  // TODO: for now load the entire file, make it better later
  // see TODO for source.h class
  return std::string((std::istreambuf_iterator<char>(in_file)),
                     (std::istreambuf_iterator<char>()));
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    return 1;
  }

  check_out_dir({argv[2]});

  // TODO: make the Source work with files or iterators
  std::string content = get_input_content(argv[1]);
  std::vector<Source> sources = {{std::move(content)}};

  auto static_ref = [](auto& p) {
    return static_reflection::StaticReflexParser{p};
  };
  auto std_parser = [](auto& p) { return std_parser::StdParser{}; };
  Preprocessor preprocessor(std::move(sources), static_ref, std_parser);

  std::ofstream out_file(argv[2], std::ios::out);
  // TODO: need to provide a writer for the processed content
  auto writer = [&out_file](auto& src) {
    for (auto& elem : src) {
      out_file << elem;
    }
  };
  preprocessor.process(writer);

  std::cout << "DONE" << std::endl;

  return 0;
}
