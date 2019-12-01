#ifndef META_CLASSES_H
#define META_CLASSES_H

#include <algorithm>
#include <iostream>
#include <iterator>
#include <unordered_set>
#include <variant>

#include <overloaded.hpp>
#include <result.hpp>
#include <source_loader.hpp>
#include <std_ast.hpp>
#include <string_utils.hpp>

#include <gen_utils.hpp>
#include <meta_classes_rules.hpp>
#include <meta_process.hpp>

#include <boost/process.hpp>

namespace bp = boost::process;
namespace std_ast = std_parser::rules::ast;

namespace meta_classes {
template <class Parent>
class MetaClassParser {
  template <class Source>
  using RetType = std::optional<Result<Iter<Source>, std::string>>;

  template <class T>
  auto make_result(T out) {
    return out ? std::optional{Result{out->processed_to, std::string()}}
               : std::nullopt;
  }

  template <class Source>
  RetType<Source> make_empty_result() {
    return std::nullopt;
  }

  bool is_meta_param(std_ast::var const& var) {
    // TODO: check for aliases when we can
    auto const& type_name = var.type.type.type.front().name;
    return type_name == std::vector<std::string>{"meta", "type"};
  }

  bool is_only_const(std_ast::var const& var) {
    auto const& type = var.type;
    auto const& lq = type.left_qualifiers;
    auto const& rq = type.right_qualifiers;
    if (lq.size() + rq.size() != 1) {
      return false;
    }

    if (not lq.empty() and lq.front() == std_ast::TypeQualifier::Const) {
      return true;
    }

    if (lq.front() == std_ast::TypeQualifier::Const) {
      return true;
    }

    return false;
  }

  /**
   * checks if it is a meta function
   * needs to be constexpr,
   * have 2 parameters of type meta::type
   * and the second needs to be const
   */
  bool is_meta_function(std_ast::Function const& fun) {
    bool is_constexpr_function = fun.is_constexpr;
    if (not is_constexpr_function) {
      return false;
    }

    auto const& params = fun.parameters.parameters;
    bool has_two_parameters = params.size() == 2;
    if (not has_two_parameters) {
      return false;
    }

    return is_meta_param(params[0]) and is_meta_param(params[1]) and
           is_only_const(params[1]);
  }

  template <class Source, class Writer>
  RetType<Source> parse_constexpr_function(Source& source, Writer& writer) {
    auto begin = source.begin();
    auto end = source.end();

    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto out = std_parser.try_parse_function(begin, end);

    if (out) {
      bool is_meta = is_meta_function(out->result);

      if (is_meta) {
        auto& fun = out->result;
        meta_classes.emplace(fun.name);
        inside_meta_class_function = true;
        std::string res = {source.begin(), out->processed_to};
        const std::string c = "constexpr";
        auto it = std::search(res.begin(), res.end(), c.begin(), c.end());
        res.erase(it, it + c.size());

        writer(res);
        using Fun = std_parser::rules::ast::Function;

        std::size_t processed_chars = 0;
        do {
          source.advance(processed_chars);

          auto out = std_parser.parse(source);
          if (out) {
            processed_chars = std::distance(source.begin(), out->processed_to);
            if (processed_chars == 0) {
              throw std::runtime_error("error in one of the parsers 2");
            }
          } else {
            return make_empty_result<Source>();
          }
        } while (not std::holds_alternative<Fun>(
            std_parser.get_current_code_fragment()));

        return Result{source.begin() + processed_chars, std::string()};
      }

      return std::nullopt;
    }

    return make_empty_result<Source>();
  }

  template <class Source>
  RetType<Source> parse_meta_class(Source& source) {
    bool is_parsed = false;
    auto begin = source.begin();
    auto end = source.end();
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();

    auto tmpls = std_parser.try_parse_templates_params(begin, end);
    begin = tmpls ? tmpls->processed_to : begin;
    auto meta_name = std_parser.try_parse_name(begin, end);

    bool is_known_meta_class =
        meta_name && meta_classes.count(meta_name->result);
    // NOTE: while used just to use break instead of goto
    // Maybe use a lambda
    while (is_known_meta_class) {
      begin = meta_name->processed_to;

      auto class_name = std_parser.try_parse_name(begin, end);
      if (!class_name) {
        break;
      }
      begin = class_name->processed_to;
      auto class_bases = std_parser.try_parse_class_bases(begin, end);
      if (class_bases) {
        begin = class_bases->processed_to;
      }

      auto scope_begin = std_parser.try_parse_scope_begin(begin, end);
      if (!scope_begin) {
        break;
      }
      begin = scope_begin->processed_to;

      using Class = std_parser::rules::ast::class_or_struct;
      Class cls;
      if (tmpls) {
        cls.template_parameters = std::move(tmpls->result);
      }
      cls.name = class_name->result;
      cls.type = std_parser::rules::ast::class_type::META_CLASS;
      if (class_bases) {
        cls.bases = std::move(class_bases->result);
      }
      std_parser.open_new_code_fragment(std::move(cls));
      is_parsed = true;
      current_meta_class = std::move(meta_name->result);
      current_meta_class_name = std::move(class_name->result);
      break;
    }

    return is_parsed ? std::optional{Result{begin, std::string{}}}
                     : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_include(Source& source, Writer& writer) {
    auto begin = source.begin();
    auto end = source.end();

    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto out = std_parser.parse_include(source);

    if (out) {
      std::string inc(source.begin(), out->processed_to);
      writer(inc);
    }

    return out ? std::optional{Result{out->processed_to,
                                      std::string{out->result}}}
               : std::nullopt;
  }

  template <class Source, class Writer>
  RetType<Source> parse_meta(Source& source, Writer& writer) {
    auto out = parse_constexpr_function(source, writer);
    if (out) {
      return out;
    }

    return parse_meta_class(source);
  }

  template <class Source, class Writer>
  auto parse_target(Source& source, Writer& writer) {
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
      out = gen_target_output(t, source.begin(), begin);
      writer(out);
    }

    return parsed ? std::optional{Result{begin, std::string()}} : std::nullopt;
  }

  /**
   * Traverse the current code_fragments and try to find a constexpr function
   */
  bool is_still_inside_constexpr_function() {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto& code_fragments = std_parser.get_all_code_fragments();

    using Fun = std_parser::rules::ast::Function;

    return std::any_of(code_fragments.begin(), code_fragments.end(),
                       [this](auto& code_fragment) {
                         // TODO: when parsing a constexpr meta function save
                         // it's name and check it here
                         return std::holds_alternative<Fun>(code_fragment);
                       });
  }

  template <class Source, class Writer>
  auto parse_inside_constexpr_function(Source& source, Writer& writer) {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto out = std_parser.parse(source);
    if (out) {
      writer(out->result);

      if (!is_still_inside_constexpr_function()) {
        inside_meta_class_function = false;
      }
    }

    return out ? make_result(out) : parse_target(source, writer);
  }

  /**
   * Traverse the current code_fragments and try to find a class with our
   * current_meta_class_name
   */
  bool is_still_inside_meta_class() {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto& code_fragments = std_parser.get_all_code_fragments();

    using Class = std_parser::rules::ast::Class;

    return std::any_of(
        code_fragments.begin(), code_fragments.end(),
        [this](auto& code_fragment) {
          if (!std::holds_alternative<Class>(code_fragment)) {
            return false;
          }
          auto class_code_fragment = std::get<Class>(code_fragment);
          return class_code_fragment.name == current_meta_class_name;
        });
  }

  template <class Source>
  auto parse_inside_meta_class(Source& source) {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end, rules::scope_end);

    std::string output;
    using Class = std_parser::rules::ast::Class;
    // TODO: check if this will get triggered if a method ends with };
    if (parsed && meta_process.ok()) {
      auto& current_code_fragment = std_parser.get_current_code_fragment();
      auto& cls = std::get<Class>(current_code_fragment);
      auto& reporter = parent.get_reporter();
      auto const& file_name = parent.get_current_file_name();
      auto error_reporter = [&reporter, &file_name](std::string_view msg) {
        reporter(file_name, msg);
      };
      output = gen_meta_class(meta_process, current_meta_class, cls, std_parser,
                              error_reporter);
    }

    auto out = std_parser.parse(source);
    if (out) {
      if (not is_still_inside_meta_class()) {
        current_meta_class.clear();
        current_meta_class_name.clear();
        return std::optional{Result{out->processed_to, output}};
      }
    }

    // TODO: if not parsed throw error ?
    return make_result(out);
  }

  Parent& parent;

  std::string meta_exe;
  std::unordered_set<std::string> meta_classes;
  bool inside_meta_class_function = false;
  std::string current_meta_class;
  std::string current_meta_class_name;
  std::ofstream out_file;
  source::SourceLoader source_loader;
  MetaProcess meta_process;
  bool is_source = false;

 public:
  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 7;

  MetaClassParser(Parent& p, std::string_view meta_exe,
                  std::string_view meta_out)
      : parent{p}, meta_exe{meta_exe}, source_loader{{}, meta_out} {
    if (!this->meta_exe.empty()) {
      meta_process = MetaProcess(this->meta_exe);
      auto& p1 = meta_process.output;
      auto& p2 = meta_process.input;
      p1 << '1' << std::endl;
      std::string out;
      int n;
      p2 >> n;
      while (n-- && p2 >> out) {
        meta_classes.emplace(std::move(out));
      }
    }
  }

  MetaClassParser(MetaClassParser&&) = default;

  ~MetaClassParser() noexcept {
    if (!this->meta_exe.empty()) {
      meta_process.output << 3 << std::endl;
      meta_process.wait();
    }
  }

  void start_preprocess(std::string_view source_name) {
    out_file = source_loader.open_source(source_name);
    out_file << "#include <meta.hpp>" << std::endl;
    is_source = source::is_source(source_name);
  }

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

    // TODO: fix this

    if (inside_meta_class_function) {
      return parse_inside_constexpr_function(source, writer);
    }
    if (!current_meta_class.empty()) {
      return parse_inside_meta_class(source);
    }

    auto meta = parse_meta(source, writer);
    return meta ? meta : parse_include(source, writer);
  }

  void finish_preprocess() {
    if (is_source) {
      out_file << gen_main(meta_classes);
    }
    out_file.close();
  }

  /**
   * Parse a Meta class
   */
  template <class Source>
  RetType<Source> parse(Source& source) {
    // TODO: fix this
    auto writer = [](auto&) {};
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

#endif  // META_CLASSES_H
