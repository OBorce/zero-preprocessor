#include <array>
#include <iostream>
#include <string>

#include <meta_classes_rules.hpp>

#include <catch.hpp>

using std::string_literals::operator""s;
namespace rules = meta_classes::rules;

template <typename Rule>
class CanParse : public Catch::MatcherBase<std::string> {
  Rule rule;
  std::string rule_name;

 public:
  CanParse(Rule r, std::string_view rule_name)
      : rule(r), rule_name{rule_name} {}

  // Performs the test for this matcher
  virtual bool match(std::string const& str) const override {
    namespace x3 = boost::spirit::x3;

    auto begin = str.begin();
    auto end = str.end();
    auto w = [this](auto& ctx) { std::cout << rule_name << " " << std::endl; };

    bool r = x3::parse(begin, end, rule[w]);
    return r && begin == end;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "Trying to be parsed as a/an " << rule_name;
    return ss.str();
  }
};

TEST_CASE("Parse valid meta class selected target", "[meta_class]") {
  std::array valid_targets{"->(target)"s, "->(some_meta_target2)"s};

  for (auto& valid_target : valid_targets) {
    REQUIRE_THAT(valid_target,
                 CanParse(rules::selected_target, "selected_target"));
  }
}

TEST_CASE("Parse valid meta class meta target", "[meta_class]") {
  std::array valid_meta_targets{"f.name()$"s, "fasd_ds.name_ds()$"s};

  for (auto& valid_meta_target : valid_meta_targets) {
    REQUIRE_THAT(valid_meta_target,
                 CanParse(rules::meta_target, "meta target"));
  }
}

TEST_CASE("Parse valid meta class meta target outputs", "[meta_class]") {
  std::array valid_target_outputs{"int a;"s, "{ fasd_ds.name_ds()$ }"s,
                                  "{ void fasd_ds.name_ds()$ {} }"s};

  for (auto& valid_target_output : valid_target_outputs) {
    REQUIRE_THAT(valid_target_output,
                 CanParse(rules::meta_target_out, "meta target output"));
  }
}

TEST_CASE("Parse valid meta class target outputs", "[meta_class]") {
  std::array valid_target_outputs{"{ int a };"s, "{ fasd_ds.name_ds()$ };"s,
                                  "{ void fasd_ds.name_ds()$ {} };"s};

  for (auto& valid_target_output : valid_target_outputs) {
    REQUIRE_THAT(valid_target_output,
                 CanParse(rules::target_out, "target output"));
  }
}

TEST_CASE("Parse valid meta class target", "[meta_class]") {
  std::array valid_targets{
      "->(target) { fasd_ds.name_ds()$ };"s,
      "  ->(target) { void fasd_ds.name_ds()$ {} };"s,
      "->(target) { virtual ~(source.name()$)() noexcept {}};"s,
      "->(target)f;"s
  };

  for (auto& valid_target : valid_targets) {
    REQUIRE_THAT(valid_target, CanParse(rules::target, "target"));
  }
}
