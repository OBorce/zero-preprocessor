#include <fstream>
#include <iostream>
// TODO: update when moved to GCC 8
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <preprocessor.h>
#include <source.h>
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

int main(int argc, char* argv[]) {
  if (argc != 3) {
    return 1;
  }

  check_out_dir({argv[2]});

  // TODO: for now just copy the files as they are
  {
    std::ifstream in_file(argv[1]);
    std::ofstream out_file(argv[2], std::ios::out);

    std::string line;
    while (std::getline(in_file, line)) {
      out_file << line << '\n';
    }
  }

  auto l = [](auto& p) { return std_parser::StdParser{}; };
  // TODO: make the Source work with files or iterators
  std::vector<Source> sources = {};
  Preprocessor preprocessor(std::move(sources), l);

  preprocessor.process();

  std::cout << "DONE" << std::endl;

  return 0;
}
