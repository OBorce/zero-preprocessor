#include <fstream>
#include <iostream>

#include <meta_classes.hpp>
#include <preprocessor.hpp>
#include <source_loader.hpp>
#include <static_reflection.hpp>
#include <std_parser.hpp>

int stage_one(int argc, char* argv[]) {
  source::check_out_dir({argv[3]});

  std::vector<std::string> inc_dirs;
  for (int i = 4; i < argc; ++i) {
    std::cout << "include:   " << argv[i] << '\n';
    inc_dirs.emplace_back(argv[i]);
  }

  source::SourceLoader loader{std::move(inc_dirs), "include"};

  auto std_parser = [](auto&) { return std_parser::StdParser{}; };

  auto meta_classes = [](auto& parent) {
    return meta_classes::MetaClassParser{parent, "", ""};
  };
  Preprocessor preprocessor(std::move(loader), std_parser);

  std::ofstream out_file(argv[3], std::ios::out);
  auto writer = [&out_file](auto& src) {
    for (auto& elem : src) {
      out_file << elem;
    }
    out_file << '\n';
  };
  preprocessor.get_dependencies(argv[2], writer);

  return 0;
}

auto read_sources(std::string_view file) {
  std::cout << "reading sources to process \n";
  std::vector<std::pair<std::string, std::string>> sources;
  std::ifstream in_file(file.data());
  std::string first, second;
  while (in_file >> first >> second) {
    std::cout << "read: " << first << " , " << second << std::endl;
    sources.emplace_back(std::move(first), std::move(second));
  }

  return sources;
}

int stage_two(int argc, char* argv[]) {
  source::check_out_dir({argv[3]});

  auto includes = read_sources(argv[3]);
  std::vector<std::string> sources;
  for (auto& s : includes) {
    sources.push_back(s.first);
  }
  sources.emplace_back(argv[2]);

  source::SourceLoader loader{{}, "include"};

  auto std_parser = [](auto&) { return std_parser::StdParser{}; };
  auto meta_classes = [&](auto& parent) {
    return meta_classes::MetaClassParser{parent, "", argv[4]};
  };

  Preprocessor preprocessor(std::move(loader), meta_classes, std_parser);

  for (auto& source : sources) {
    std::cout << "preprocessing " << source << std::endl;
    preprocessor.preprocess_source(source);
  }

  return 0;
}

int stage_three(int argc, char* argv[]) {
  if (argc != 6) {
    return 1;
  }

  auto sources = read_sources(argv[4]);
  sources.emplace_back(argv[2], argv[3]);

  source::SourceLoader loader{{}, "include"};

  auto meta_classes = [&](auto& parent) {
    return meta_classes::MetaClassParser{parent, argv[5], ""};
  };

  auto static_ref = [](auto& parent) {
    return static_reflection::StaticReflexParser{parent};
  };

  auto std_parser = [](auto&) { return std_parser::StdParser{}; };
  Preprocessor preprocessor(std::move(loader), meta_classes, static_ref,
                            std_parser);

  for (auto& pair : sources) {
    std::cout << "processing " << pair.first << " into " << pair.second
              << std::endl;
    source::check_out_dir(pair.second);
    std::ofstream out_file(pair.second, std::ios::out);
    // TODO: need to provide a writer for the processed content
    auto writer = [&out_file](auto& src) {
      for (auto& elem : src) {
        out_file << elem;
      }
    };
    preprocessor.process_source(pair.first, writer);
  }

  std::cout << "DONE" << std::endl;
  return 0;
}

int main(int argc, char* argv[]) {
  std::cout << "start\n";
  if (argc == 1) {
    return 1;
  }

  int stage = std::atoi(argv[1]);
  std::cout << "stage" << stage << "\n";

  switch (stage) {
    case 1:
      return stage_one(argc, argv);
      break;
    case 2:
      return stage_two(argc, argv);
      break;
    case 3:
      return stage_three(argc, argv);
      break;
    default:
      return 1;
  }

  return 0;
}
