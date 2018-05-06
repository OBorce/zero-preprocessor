#ifndef SOURCE_LOADER_H
#define SOURCE_LOADER_H
// TODO: update when moved to GCC 8
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include <source.h>

namespace fs = std::experimental::filesystem;

namespace source {

/**
 * Check if the output file parent dir exists
 * and create it if it doesn't
 */
void check_out_dir(fs::path out) {
  if (!out.has_parent_path()) {
    return;
  }

  auto dir = out.parent_path();
  if (!fs::exists(dir)) {
    std::cout<<"creating dirs for " << dir << std::endl;
    fs::create_directories(dir);
  }
}

/**
 * Check if include is from the standard library
 */
bool is_standard(std::string_view out) {
  // FIXME: for now just check if it contains a .
  return out.find('.') == std::string_view::npos;
}

class SourceLoader {
  std::vector<std::string> include_dirs;
  fs::path out;

 public:
  SourceLoader(std::vector<std::string>&& include_dirs, fs::path out)
      : include_dirs{std::move(include_dirs)}, out{out} {}

  SourceLoader(SourceLoader&& s)
      : include_dirs{std::move(s.include_dirs)}, out{std::move(s.out)} {}

  std::optional<fs::path> find_source(fs::path in) {
    for (auto& dir : include_dirs) {
      auto source = dir / in;

      if (fs::exists(source)) {
        return source;
      }
    }

    return std::nullopt;
  }

  fs::path get_out_path(fs::path in) { return out / in; }

  Source load_source(fs::path in) {
    std::ifstream in_file(in.c_str());
    if (!in_file.is_open()) {
      std::cerr << in << " file can't be oppened" << std::endl;
      std::terminate();
    }

    // TODO: for now load the entire file, make it better later
    return {std::string((std::istreambuf_iterator<char>(in_file)),
                        (std::istreambuf_iterator<char>())),
            in.string()};
  }
};

}  // namespace source

#endif  //! SOURCE_LOADER_H
