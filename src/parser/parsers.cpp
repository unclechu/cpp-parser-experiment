#include <string>

#include "parser/parsers.hpp"
#include "parser/applicative.hpp"

// A helper
string char_as_str(char c)
{
	string s(1, c);
	return s;
}

Parser<Unit> end_of_input()
{
	return Parser<Unit>{[](Input input) -> ParsingResult<Unit> {
		if (input.empty())
			return make_tuple(Unit{}, input);
		else
			return ParsingError{"end_of_input: input is not empty"};
	}};
}

Parser<char> any_char()
{
	return Parser<char>{[](Input input) -> ParsingResult<char> {
		if (input.empty())
			return ParsingError{"any_char: input is empty"};
		else
			return make_tuple(input[0], input.substr(1));
	}};
}

Parser<char> parse_char(char c)
{
	return Parser<char>{[=](Input input) -> ParsingResult<char> {
		auto pfx = [=](string msg){
			return "parse_char('" + char_as_str(c) + "'): " + msg;
		};

		if (input.empty())
			return ParsingError{pfx("input is empty")};
		else if (input[0] != c)
			return ParsingError{pfx(
				"char is different, got this: '" + char_as_str(input[0]) + "'"
			)};
		else
			return make_tuple(input[0], input.substr(1));
	}};
}

// Just takes what’s left in the input
Parser<string> any_string()
{
	return Parser<string>{[](Input input) { return make_tuple(input, ""); }};
}

Parser<string> parse_string(string s)
{
	return Parser<string>{[=](Input input) -> ParsingResult<string> {
		auto pfx = [=](string msg){
			return "parse_string(\"" + s + "\"): " + msg;
		};

		string taken_string;

		if (input.empty())
			return ParsingError{pfx("input is empty")};
		else if (input.size() < s.size())
			return ParsingError{pfx(
				"input is less than string (input is: \"" + input + "\")"
			)};
		else if ((taken_string = input.substr(0, s.size())) != s)
			return ParsingError{pfx(
				"string is different, got this: \"" + taken_string + "\""
			)};
		else
			return make_tuple(taken_string, input.substr(s.size()));
	}};
}

// TODO implement
Parser<int> unsigned_decimal()
{
	return pure(123);
}

// TODO implement
Parser<int> signed_decimal()
{
	return pure(123);
}

// TODO implement
Parser<double> unsigned_fractional()
{
	return pure(123.0);
}

// TODO implement
Parser<double> signed_fractional()
{
	return pure(123.0);
}
