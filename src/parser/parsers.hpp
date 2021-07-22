#ifndef _PARSER_PARSERS_HPP_
#define _PARSER_PARSERS_HPP_

// Building blocks for writing more complex parsers

#include <string>

#include "helpers.hpp"
#include "parser/types.hpp"

std::string char_as_str(char c);
Parser<Unit> end_of_input();
Parser<char> any_char();
Parser<char> parse_char(char c);
Parser<string> any_string();
Parser<string> parse_string(string s);
Parser<unsigned int> unsigned_decimal();
Parser<int> signed_decimal();
Parser<double> unsigned_fractional();
Parser<double> signed_fractional();

#endif
