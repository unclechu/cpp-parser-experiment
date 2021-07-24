#ifndef _PARSER_PARSERS_HPP_
#define _PARSER_PARSERS_HPP_

// Building blocks for writing more complex parsers

#include <functional>
#include <string>

#include "abstractions/alternative.hpp"
#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


string char_as_str(char c);
Parser<Unit> end_of_input();
Parser<char> any_char();
Parser<char> char_(char c);
Parser<char> not_char(char c);
Parser<char> satisfy(function<bool(char)>);
Parser<char> digit();

template <typename N>
// “+” or “-” sign (transforms into either identity or negate function)
Parser<function<N(N)>> num_sign()
{
	return
		(function(negative<N>) <= char_('-')) ||
		(function(id<N>)       <= char_('+'));
}

template <typename N>
Parser<function<N(N)>> optional_num_sign()
{
	return num_sign<N>() || pure(function(id<N>));
}

Parser<string> string_(string s);
Parser<string> digits();
Parser<unsigned int> unsigned_decimal();
Parser<int> signed_decimal();
Parser<double> unsigned_fractional();
Parser<double> signed_fractional();

template <typename T>
// Resolved to empty list by default
Parser<vector<T>> optional_list(Parser<vector<T>> parser)
{
	vector<T> empty_list;
	return parser || pure<decltype(empty_list)>(empty_list);
}

#endif
