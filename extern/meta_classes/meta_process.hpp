#ifndef META_PROCESS_H
#define META_PROCESS_H

#include <boost/process.hpp>

namespace bp = boost::process;

namespace meta_classes {
class MetaProcess {
  bp::child process;

 public:
  bp::opstream output;
  bp::ipstream input;

  MetaProcess() = default;

  MetaProcess(std::string_view process_name) {
    if (!process_name.empty()) {
      this->process = bp::child(process_name.data(), bp::std_out > input,
                                bp::std_in < output);
    }
  }

  // NOTE: something is wrong with the defaut move of basic_opstream so we move
  // the pipe
  MetaProcess(MetaProcess&& p)
      : process{std::move(p.process)},
        output{std::move(p.output.pipe())},
        input{std::move(p.input.pipe())} {}

  MetaProcess& operator=(MetaProcess&& p) {
    process = std::move(p.process);
    output = std::move(p.output.pipe());
    input = std::move(p.input.pipe());

    return *this;
  }

  bool ok() { return process.running(); }

  void wait() noexcept {
    std::error_code e;
    process.wait(e);
  }
};
}  // namespace meta_classes

#endif  //! META_PROCESS_H
