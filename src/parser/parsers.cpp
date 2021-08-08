#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include "abstractions/alternative.hpp"
#include "abstractions/applicative.hpp"
#include "helpers.hpp"
#include "parser/parsers.hpp"

using namespace std;

// Local shorthand
using I = ParserInputType<Parser>;


// A helper
string char_as_str(char c)
{
	string s(1, c);
	return s;
}

// endOfInput :: forall t . Chunk t => Parser t ()
Parser<Unit> end_of_input()
{
	return Parser<Unit>{[](I input) -> ParsingResult<Unit, I> {
		if (input.empty())
			return make_parsing_success<Unit, I>(unit(), input);
		else
			return make_parsing_error<I>("end_of_input: input is not empty", input);
	}};
}

// anyChar :: Parser Char
Parser<char> any_char()
{
	return Parser<char>{[](I input) -> ParsingResult<char, I> {
		if (input.empty())
			return make_parsing_error<I>("any_char: input is empty", input);
		else
			return make_parsing_success<char, I>(input[0], input.substr(1));
	}};
}

// char :: Char -> Parser Char
Parser<char> char_(char c)
{
	return prefix_parsing_failure(
		"char_('" + char_as_str(c) + "')",
		Parser<char>{[=](I input) -> ParsingResult<char, I> {
			if (input.empty())
				return make_parsing_error<I>("input is empty", input);
			else if (input[0] != c)
				return make_parsing_error<I>(
					"char is different, got this: '" +
					char_as_str(input[0]) + "'",
					input
				);
			else
				return make_parsing_success<char, I>(input[0], input.substr(1));
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
	return Parser<char>{[predicate](I input) -> ParsingResult<char, I> {
		if (input.empty())
			return make_parsing_error<I>("satisfy: input is empty", input);
		else if (predicate(input[0]))
			return make_parsing_success<char, I>(input[0], input.substr(1));
		else
			return make_parsing_error<I>(
				"satisfy: '" + char_as_str(input[0]) +
				"' does not satisfy predicate",
				input
			);
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
		Parser<string>{[=](I input) -> ParsingResult<string, I> {
			string taken_string;

			if (input.empty())
				return make_parsing_error<I>("Input is empty", input);
			else if (input.size() < s.size())
				return make_parsing_error<I>(
					"Input is less than string (input is: \"" + input + "\")",
					input
				);
			else if ((taken_string = input.substr(0, s.size())) != s)
				return make_parsing_error<I>(
					"String is different, got this: \"" + taken_string + "\"",
					input
				);
			else
				return make_parsing_success<string, I>(
					taken_string,
					input.substr(s.size())
				);
		}}
	);
}

Parser<string> digits()
{
	return prefix_parsing_failure(
		"digits",
		function(chars_to_string<vector>) ^ some(digit())
	);
}

template <typename T>
inline Parser<T> generic_decimal_parser(string parser_name)
{
	return prefix_parsing_failure(
		parser_name,
		Parser<T>{[](I input) -> ParsingResult<T, I> {
			return visit(overloaded {
				[](ParsingError<I> err) -> ParsingResult<T, I> { return err; },
				[input](ParsingSuccess<string, I> x) -> ParsingResult<T, I> {
					auto [ digits_str, tail ] = x;
					try {
						return make_parsing_success<T, I>(
							stoi(digits_str),
							tail
						);
					} catch (out_of_range&) {
						return make_parsing_error<I>(
							"Integer value is out of integer bounds: " +
							digits_str,
							input
						);
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
		Parser<T>{[fractional_number](I input) -> ParsingResult<T, I> {
			return visit(overloaded {
				[](ParsingError<I> err) -> ParsingResult<T, I> { return err; },
				[input](ParsingSuccess<string, I> x) -> ParsingResult<T, I> {
					auto [ number_str, tail ] = x;
					try {
						return make_parsing_success<T, I>(
							stod(number_str),
							tail
						);
					} catch (out_of_range&) {
						return make_parsing_error<I>(
							"Fractional value is out of type bounds: " +
							number_str,
							input
						);
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
