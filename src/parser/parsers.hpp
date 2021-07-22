#ifndef _PARSER_PARSERS_HPP_
#define _PARSER_PARSERS_HPP_

// Building blocks for writing more complex parsers

#include <string>

#include "helpers.hpp"
#include "parser/alternative.hpp"
#include "parser/types.hpp"

std::string char_as_str(char c);
Parser<Unit> end_of_input();
Parser<char> any_char();
Parser<char> parse_char(char c);
Parser<char> not_char(char c);
Parser<char> satisfy(function<bool(char)>);
Parser<char> digit();

template <typename N>
// “+” or “-” sign (transforms into either identity or negate function)
Parser<function<N(N)>> num_sign()
{
	return
		(function(negative<N>) <= parse_char('-')) ||
		(function(id<N>)       <= parse_char('+'));
}

template <typename N>
Parser<function<N(N)>> optional_num_sign()
{
	return num_sign<N>() || pure(function(id<N>));
}

Parser<string> parse_string(string s);
Parser<string> digits();
Parser<unsigned int> unsigned_decimal();
Parser<int> signed_decimal();
Parser<double> unsigned_fractional();
Parser<double> signed_fractional();

#endif
