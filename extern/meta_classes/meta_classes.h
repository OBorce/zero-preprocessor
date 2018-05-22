#ifndef META_CLASSES_H
#define META_CLASSES_H

#include <algorithm>
#include <iostream>
#include <iterator>
#include <unordered_set>
#include <variant>

#include <overloaded.h>
#include <result.h>
#include <std_ast.h>
#include <string_utils.h>

#include <gen_utils.h>
#include <meta_classes_rules.h>
#include <meta_process.h>

#include <boost/process.hpp>

namespace bp = boost::process;

namespace meta_classes {
template <typename Parent>
class MetaClassParser {
  template <class T>
  auto make_result(T out) {
    return out ? std::optional{Result{(*out).processed_to, std::string()}}
               : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_meta_calss_function(Source& source, Writer& writer) {
    std::cout << "parsing meta class function\n";
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();

    const auto out = std_parser.parse(source);
    std::string res;
    if (out) {
      // if inside a constexpr function add it as a meta function
      std::cout << "sucess parsed a function\n";
      const auto& current_nesting = std_parser.get_current_nesting();
      using Fun = std_parser::rules::ast::Function;
      if (std::holds_alternative<Fun>(current_nesting)) {
        const Fun& fun = std::get<Fun>(current_nesting);
        meta_classes.emplace(fun.name);
        inside_meta_class_function = true;
        res = {(*out).result.begin(), (*out).result.end()};
        const std::string c = "constexpr";
        auto it = std::search(res.begin(), res.end(), c.begin(), c.end());
        res.erase(it, it + c.size());

        writer(res);
      }
    }

    return out ? std::optional{Result{(*out).processed_to, std::string()}}
               : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_constexpr_function(Source& source, Writer& writer) {
    std::cout << "trying to parse constexpr\n";
    auto begin = source.begin();
    auto end = source.end();

    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto out = std_parser.parse_function(source);

    if (out) {
      std::cout << "parsed a function\n";
      std::string res(source.begin(), (*out).processed_to);
      // TODO: check for params to be meta::type
      bool is_constexpr_function = res.find("constexpr") != std::string::npos;

      return is_constexpr_function ? parse_meta_calss_function(source, writer)
                                   : std::nullopt;
    }

    return make_result(out);
  }

  template <class Source>
  auto parse_meta_class(Source& source) {
    std::cout << "trying to parse meta class\n";
    auto begin = source.begin();
    auto end = source.end();

    rules::ast::MetaClass m;
    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end,
                            // begin rules
                            rules::meta_class,
                            // end rules
                            m);

    bool is_known_meta_class = parsed && meta_classes.count(m.meta_class_name);
    if (is_known_meta_class) {
      auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
      current_meta_class = m.meta_class_name;
      using Class = std_parser::rules::ast::Class;
      // TODO: add class templates
      std_parser.open_new_nesting(Class{std::move(m.name)});
    }

    return is_known_meta_class ? std::optional{Result{begin, std::string{}}}
                               : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_meta(Source& source, Writer& writer) {
    auto out = parse_constexpr_function(source, writer);

    return out ? out : parse_meta_class(source);
  }

  template <class Source, class Writer>
  auto parse_target(Source& source, Writer& writer) {
    std::cout << "trying to parse meta class target output\n";
    auto begin = source.begin();
    auto end = source.end();

    rules::ast::Target t;
    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end,
                            // begin rules
                            rules::target,
                            // end rules
                            t);

    std::string out;
    if (parsed) {
      std::cout << "parsed meta class target" << t.target << " " << std::endl;
      for (auto& out : t.output) {
        std::cout << out << "; ";
      }
      out = gen_target_output(t, source.begin(), begin);
      writer(out);
    }

    return parsed ? std::optional{Result{begin, std::string()}} : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_inside_constexpr_function(Source& source, Writer& writer) {
    std::cout << "parsing inside constexpr function\n";
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto out = std_parser.parse(source);
    if (out) {
      writer((*out).result);

      // if end of function inside_meta_class_function = false;
      auto& current_nesting = std_parser.get_current_nesting();
      using Fun = std_parser::rules::ast::Function;
      using Scope = std_parser::rules::ast::Scope;
      if (!std::holds_alternative<Fun>(current_nesting) &&
          !std::holds_alternative<Scope>(current_nesting)) {
        inside_meta_class_function = false;
      }
    }

    return out ? make_result(out) : parse_target(source, writer);
  }

  template <class Source>
  auto parse_inside_meta_class(Source& source) {
    std::cout << "parsing inside meta class\n";

    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end, rules::scope_end);

    std::string output;
    using Class = std_parser::rules::ast::Class;
    if (parsed && meta_process.ok()) {
      std::cout << "parsed end of meta class\n";
      auto& current_nesting = std_parser.get_current_nesting();
      auto& cls = std::get<Class>(current_nesting);
      output = std::move(gen_meta_class(meta_process, current_meta_class, cls));
    }

    auto out = std_parser.parse(source);
    if (out) {
      auto& current_nesting = std_parser.get_current_nesting();
      if (!std::holds_alternative<Class>(current_nesting)) {
        current_meta_class.clear();
        return std::optional{Result{(*out).processed_to, output}};
      }
    }

    // TODO: if not parsed throw error ?
    return make_result(out);
  }

  Parent& parent;
  std::string meta_exe;
  std::string meta_out;
  std::unordered_set<std::string> meta_classes;
  bool inside_meta_class_function = false;
  std::string current_meta_class;
  bool first_line = true;
  std::ofstream out_file;
  MetaProcess meta_process;

 public:
  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 7;

  MetaClassParser(Parent& p, std::string_view meta_exe,
                  std::string_view meta_out)
      : parent{p}, meta_exe{meta_exe}, meta_out{meta_out} {
    if (!this->meta_exe.empty()) {
      meta_process = MetaProcess(this->meta_exe);
      std::cout << "reading meta from process " << this->meta_exe << "\n";
      bp::opstream& p1 = meta_process.output;
      bp::ipstream& p2 = meta_process.input;
      p1 << '1' << std::endl;
      std::string out;
      int n;
      p2 >> n;
      while (n-- && p2 >> out) {
        std::cout << "read meta output: " << out << std::endl;
        meta_classes.emplace(std::move(out));
      }
    }

    // TODO: use a SourceManager
    if (!this->meta_out.empty()) {
      out_file = std::ofstream(this->meta_out.data(), std::ios::out);
    }
  }

  MetaClassParser(MetaClassParser&&) = default;

  /**
   * Parse constexpr meta class function
   * and write them
   */
  template <class Source>
  auto preprocess(Source& source) {
    auto writer = [this](auto& src) {
      for (auto& elem : src) {
        out_file << elem;
      }
      out_file << '\n';
    };

    if (first_line) {
      first_line = false;
      writer("#include <meta.h>\n");
    }

    // TODO: fix this

    if (inside_meta_class_function) {
      return parse_inside_constexpr_function(source, writer);
    }
    if (!current_meta_class.empty()) {
      return parse_inside_meta_class(source);
    }

    return parse_meta(source, writer);
  }

  void finish_preprocess() {
    out_file << gen_main(meta_classes);
    out_file.close();
  }

  /**
   * Parse a Meta class
   */
  template <class Source>
  auto parse(Source& source) {
    // TODO: fix this
    auto writer = [](auto& a) {};
    if (inside_meta_class_function) {
      return parse_inside_constexpr_function(source, writer);
    }
    if (!current_meta_class.empty()) {
      return parse_inside_meta_class(source);
    }

    return parse_meta(source, writer);
  }
};
}  // namespace meta_classes

#endif  //! META_CLASSES_H
