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
      auto result = parser.parse(source);
      THEN("should put a class as it's current nesting") {
        REQUIRE(result.has_value());
        auto processed_to = (*result).processed_to;

        auto expected_parsed_till =
            std::find(source.begin(), source.end(), '\n');

        REQUIRE(std::distance(source.begin(), processed_to) ==
                std::distance(source.begin(), expected_parsed_till));
        auto& current_nesting = parser.get_current_nesting();

        REQUIRE(std::holds_alternative<rules::ast::Class>(current_nesting));
      }

      auto processed_to = (*result).processed_to;
      auto parsed_characters = std::distance(source.begin(), processed_to);
      source.advance(parsed_characters);
      INFO("parsed " + std::to_string(parsed_characters));
      WHEN("Parsed again") {
        auto result = parser.parse(source);
        THEN("should parse till the end and close the class") {
          REQUIRE(result.has_value());
          auto processed_to = (*result).processed_to;
          auto parsed_characters = std::distance(source.begin(), processed_to);
          INFO("parsed another " + std::to_string(parsed_characters));
          source.advance(parsed_characters);
          REQUIRE(source.is_finished());
          auto& current_nesting = parser.get_current_nesting();

          REQUIRE(
              std::holds_alternative<rules::ast::Namespace>(current_nesting));
        }
      }
    }
  }
}
