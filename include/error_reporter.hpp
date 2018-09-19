#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <iostream>
#include <string_view>

// TODO: make it a logger
struct ErrorReporter {
  void operator()(std::string_view msg) { std::cerr << msg << std::endl; }

  template <class... Args>
  void operator()(Args&&... msgs) {
    (std::cerr << ... << msgs) << std::endl;
  }
};

#endif  // ERROR_REPORTER_H
