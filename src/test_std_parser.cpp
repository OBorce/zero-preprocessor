#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include <source.h>
#include <std_parser.h>

#include "catch.hpp"

TEST_CASE("Parse a class", "[parser]") {
  using namespace std_parser;
  StdParser parser;

  GIVEN("A source containing a class") {
    Source source{R"c(class A{
};)c"};
    WHEN("Parsed by std_parser") {
      auto parsed_till = parser.parse(source);
      THEN("should put a class as it's current nesting") {
        REQUIRE(parsed_till.has_value());

        auto expected_parsed_till =
            std::find(source.begin(), source.end(), '\n');

        REQUIRE(std::distance(source.begin(), *parsed_till) ==
                std::distance(source.begin(), expected_parsed_till));
        auto& current_nesting = parser.get_current_nesting();

        REQUIRE(std::holds_alternative<rules::ast::Class>(current_nesting));
      }

      auto parsed_characters = std::distance(source.begin(), *parsed_till);
      source.advance_for(parsed_characters);
      INFO("parsed " + std::to_string(parsed_characters));
      WHEN("Parsed again") {
        auto parsed_till = parser.parse(source);
        THEN("should parse till the end and close the class") {
          REQUIRE(parsed_till.has_value());
          auto parsed_characters = std::distance(source.begin(), *parsed_till);
          INFO("parsed another " + std::to_string(parsed_characters));
          source.advance_for(parsed_characters);
          REQUIRE(source.is_finished());
          auto& current_nesting = parser.get_current_nesting();

          REQUIRE(
              std::holds_alternative<rules::ast::Namespace>(current_nesting));
        }
      }
    }
  }
}
