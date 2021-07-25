#include <functional>
#include <stdexcept>
#include <string>

#include "abstractions/alternative.hpp"
#include "abstractions/applicative.hpp"
#include "helpers.hpp"
#include "parser/parsers.hpp"

using namespace std;


// A helper
string char_as_str(char c)
{
	string s(1, c);
	return s;
}

// endOfInput :: forall t . Chunk t => Parser t ()
Parser<Unit> end_of_input()
{
	return Parser<Unit>{[](Input input) -> ParsingResult<Unit> {
		if (input.empty())
			return make_tuple(Unit{}, input);
		else
			return ParsingError{"end_of_input: input is not empty"};
	}};
}

// anyChar :: Parser Char
Parser<char> any_char()
{
	return Parser<char>{[](Input input) -> ParsingResult<char> {
		if (input.empty())
			return ParsingError{"any_char: input is empty"};
		else
			return make_tuple(input[0], input.substr(1));
	}};
}

// char :: Char -> Parser Char
Parser<char> char_(char c)
{
	return prefix_parsing_failure(
		"char_('" + char_as_str(c) + "')",
		Parser<char>{[=](Input input) -> ParsingResult<char> {
			if (input.empty())
				return ParsingError{"input is empty"};
			else if (input[0] != c)
				return ParsingError{
					"char is different, got this: '" +
					char_as_str(input[0]) + "'"
				};
			else
				return make_tuple(input[0], input.substr(1));
		}}
	);
}

// Negative version of ‘char_’.
// notChar :: Char -> Parser Char
Parser<char> not_char(char c)
{
	return satisfy([c](char x) { return x != c; });
}

// satisfy :: (Char -> Bool) -> Parser Char
Parser<char> satisfy(function<bool(char)> predicate)
{
	return Parser<char>{[predicate](Input input) -> ParsingResult<char> {
		if (input.empty())
			return ParsingError{"satisfy: input is empty"};
		else if (predicate(input[0]))
			return make_tuple(input[0], input.substr(1));
		else
			return ParsingError{
				"satisfy: '" + char_as_str(input[0]) +
				"' does not satisfy predicate"
			};
	}};
}

// digit :: Parser Char
Parser<char> digit()
{
	return satisfy([](char c) { return c >= '0' && c <= '9'; });
}

// string :: Text -> Parser Text
Parser<string> string_(string s)
{
	return prefix_parsing_failure(
		"string_(\"" + s + "\")",
		Parser<string>{[=](Input input) -> ParsingResult<string> {
			string taken_string;

			if (input.empty())
				return ParsingError{"Input is empty"};
			else if (input.size() < s.size())
				return ParsingError{
					"Input is less than string (input is: \"" + input + "\")"
				};
			else if ((taken_string = input.substr(0, s.size())) != s)
				return ParsingError{
					"String is different, got this: \"" + taken_string + "\""
				};
			else
				return make_tuple(taken_string, input.substr(s.size()));
		}}
	);
}

Parser<string> digits()
{
	return prefix_parsing_failure(
		"digits",
		function<string(vector<char>)>(chars_to_string) ^ some(digit())
	);
}

template <typename T>
inline Parser<T> generic_decimal_parser(string parser_name)
{
	return prefix_parsing_failure(
		parser_name,
		Parser<T>{[](Input input) -> ParsingResult<T> {
			return visit(overloaded {
				[](ParsingError err) -> ParsingResult <T> { return err; },
				[](ParsingSuccess<string> x) -> ParsingResult <T> {
					auto [ digits_str, tail ] = x;
					try {
						return make_tuple(stoi(digits_str), tail);
					} catch (out_of_range&) {
						return ParsingError{
							"Integer value is out of integer bounds: " +
							digits_str
						};
					}
				}
			}, digits()(input));
		}}
	);
}

Parser<unsigned int> unsigned_decimal()
{
	return generic_decimal_parser<unsigned int>("unsigned_decimal");
}

Parser<int> signed_decimal()
{
	return
		optional_num_sign<int>()
		^ generic_decimal_parser<int>("signed_decimal");
}

template <typename T>
inline Parser<T> generic_fractional_parser(string parser_name)
{
	function<string(string, char, string)> concat =
		[](string a, char b, string c) { return a + char_as_str(b) + c; };

	Parser<string> fractional_number =
		curry(concat) ^ digits() ^ char_('.') ^ digits();

	return prefix_parsing_failure(
		parser_name,
		Parser<T>{[fractional_number](Input input) -> ParsingResult<T> {
			return visit(overloaded {
				[](ParsingError err) -> ParsingResult<T> { return err; },
				[](ParsingSuccess<string> x) -> ParsingResult<T> {
					auto [ number_str, tail ] = x;
					try {
						return make_tuple(stod(number_str), tail);
					} catch (out_of_range&) {
						return ParsingError{
							"Fractional value is out of type bounds: " +
							number_str
						};
					}
				}
			}, fractional_number(input));
		}}
	);
}

Parser<double> unsigned_fractional()
{
	return generic_fractional_parser<double>("unsigned_fractional");
}

Parser<double> signed_fractional()
{
	return
		optional_num_sign<double>()
		^ generic_fractional_parser<double>("signed_fractional");
}
