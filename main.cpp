#include <iostream>
#include <functional>
#include <variant>
#include <tuple>
#include <list>
#include <memory>
#include <string>
#include <map>
#include <optional>
#include <typeinfo>

using namespace std;


// Helpers {{{1

struct Unit: tuple<> {};

// Helpers for pattern-matching-ish syntax for std::visit taken from here:
// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// }}}1


// Free JSON value representation type {{{1

struct JsonValue; // Algebraic data type

// Constructors-ish
struct JsonObject: tuple<map<string, JsonValue>> {};
struct JsonArray: tuple<list<unique_ptr<JsonValue>>> {};
struct JsonString: tuple<string> {};
struct JsonNumber: tuple<variant<int, double>> {};
struct JsonBool: tuple<bool> {};
struct JsonNull: Unit {};

struct JsonValue: variant<
	JsonObject,
	JsonArray,
	JsonString,
	JsonNumber,
	JsonBool,
	JsonNull
> {};

// }}}1


// Basic parser implementation {{{1

// Basic type {{{2

using Input = string;
struct ParsingError: string {};

template <typename A>
using ParsingSuccess = tuple<A, Input>;

template <typename A>
using ParsingResult = variant<ParsingError, ParsingSuccess<A>>;

template <typename A>
// Parser a = String → Either String (a, String)
struct Parser: function<ParsingResult<A>(Input)> {};

// }}}2

// Functor-like stuff {{{2

template <typename A, typename B>
// (<$>) :: (a → b) → Parser a → Parser b
Parser<B> fmap_parser(function<B(A)> map_fn, Parser<A> parser)
{
	return Parser<B>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingSuccess<A> x) -> ParsingResult<B> {
				auto [ value, tail ] = x;
				return make_tuple(map_fn(value), tail);
			},
			[](ParsingError err) -> ParsingResult<B> { return err; },
		}, parser(input));
	}};
}
template <typename A, typename B>
// Operator equivalent for “fmap_parser”
Parser<B> operator^(function<B(A)> map_fn, Parser<A> parser)
{
	return fmap_parser<A, B>(map_fn, parser);
}
template <typename A, typename B>
// Flipped version of “fmap” (like (<&>) comparing to (<$>))
Parser<B> operator&(Parser<A> parser, function<B(A)> map_fn)
{
	return fmap_parser<A, B>(map_fn, parser);
}

template <typename A, typename B>
// (<$) :: a → Parser b → Parser a
Parser<A> void_right(A to_value, Parser<B> parser)
{
	return fmap_parser<B, A>([=](B) { return to_value; }, parser);
}
template <typename A, typename B>
// Operator equivalent for “void_right”
Parser<A> operator<=(A to_value, Parser<B> parser)
{
	return void_right<A, B>(to_value, parser);
}

template <typename A, typename B>
// ($>) :: Parser a → b → Parser b
Parser<B> void_left(Parser<A> parser, B to_value)
{
	return void_right(to_value, parser);
}
template <typename A, typename B>
// Operator equivalent for “void_left”
Parser<B> operator>=(Parser<A> parser, B to_value)
{
	return void_left<A, B>(parser, to_value);
}

// }}}2

// Applicative-like stuff {{{2

template <typename A>
Parser<A> pure(A x)
{
	return Parser<A>{[=](Input input) { return make_tuple(x, input); }};
}

template <typename A, typename B>
// (<*>) :: Parser (a → b) → Parser a → Parser b
Parser<B> apply(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return Parser<B>{[=](Input input) -> ParsingResult<B> {
		return visit(overloaded {
			[=](ParsingSuccess<function<B(A)>> x) -> ParsingResult<B> {
				auto [ fn, tail ] = x;
				return fmap_parser<A, B>(fn, parser)(tail);
			},
			[](ParsingError err) -> ParsingResult<B> { return err; }
		}, fn_parser(input));
	}};
}
template <typename A, typename B>
// Operator equivalent for “apply”
Parser<B> operator^(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return apply<A, B>(fn_parser, parser);
}

template <typename A, typename B>
// (<*) :: Parser a → Parser b → Parser a
Parser<A> apply_first(Parser<A> parser_a, Parser<B> parser_b)
{
	// (\a _ -> a) <$> parser_a <*> parser_b
	return apply<B, A>(
		fmap_parser<A, function<A(B)>>(
			[](A a) { return [=](B) { return a; }; },
			parser_a
		),
		parser_b
	);
}
template <typename A, typename B>
// Operator equivalent for “apply_first”
Parser<A> operator<<(Parser<A> parser_a, Parser<B> parser_b)
{
	return apply_first<A, B>(parser_a, parser_b);
}

template <typename A, typename B>
// (*>) :: Parser a → Parser b → Parser b
Parser<B> apply_second(Parser<A> parser_a, Parser<B> parser_b)
{
	// (\_ b -> b) <$> parser_a <*> parser_b
	return apply<B, B>(
		fmap_parser<A, function<B(B)>>(
			[](A) { return [](B b) { return b; }; },
			parser_a
		),
		parser_b
	);
}
template <typename A, typename B>
// Operator equivalent for “apply_second”
Parser<B> operator>>(Parser<A> parser_a, Parser<B> parser_b)
{
	return apply_second<A, B>(parser_a, parser_b);
}

// }}}2

// Parsing functions {{{2

template <typename A, typename B>
// either :: (a → c) → (b → c) → Either a b → c
function<B(ParsingResult<A>)> parsing_resolver(
	function<B(ParsingError)> failure_resolve,
	function<B(A)> success_resolve
)
{
	return [=](ParsingResult<A> result) {
		return visit(overloaded {
			[=](ParsingSuccess<A> x) -> B {
				auto [ value, _ ] = x;
				return success_resolve(value);
			},
			[=](ParsingError err) -> B {
				return failure_resolve(err);
			}
		}, result);
	};
}

template <typename A, typename B>
B parse(function<B(ParsingResult<A>)> resolver, Parser<A> parser, Input input)
{
	return resolver(parser(input));
}
template <typename A>
variant<ParsingError, A> parse(Parser<A> parser, Input input)
{
	using Result = variant<ParsingError, A>;
	return parse<A, Result>(
		parsing_resolver<A, Result>(
			[](ParsingError err) { return err; },
			[](A x) { return x; }
		),
		parser,
		input
	);
}

// }}}2

// }}}1


// Atomic-ish parsers (the building blocks) {{{1

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

// }}}1


int test_basic_stuff();
int test_composition_of_simple_parsers();

int main()
{
	//TODO call depending on arguments
	//return test_basic_stuff();
	return test_composition_of_simple_parsers();
}


// Stupid WIP tests {{{1

template <typename T>
int debug_a_test(variant<ParsingError, T> result)
{
	if (auto x = get_if<ParsingError>(&result)) {
		cerr << "Failed to parse: " << *x << endl;
		return EXIT_FAILURE;
	} else {
		cout << "Successfully parsed: " << get<T>(result) << endl;
		return EXIT_SUCCESS;
	}
}

int test_basic_stuff()
{
	/*const Parser<int> test1 = Parser<int>{[](string x) {
		// return make_tuple(123, x);
		return ParsingError{"oh no!"};
	}};*/
	const Parser<int> test1 = pure(123);

	/* const Parser<string> test2 = fmap_parser<int, string>([](int x) { */
	/* 	return to_string(x); */
	/* }, test1); */
	const Parser<string> test2 =
		function<string(int)>([](int x) { return to_string(x); }) ^ test1;

	const Parser<bool> test3 = test2 >= true;

	/* const Parser<function<string(bool)>> test4 = */
	/* 	void_right<function<string(bool)>, bool>( */
	/* 		[](bool x) { return x ? "yes" : "no"; }, */
	/* 		test3 */
	/* 	); */
	const Parser<function<string(bool)>> test4 =
		function<string(bool)>([](bool x) { return x ? "yes" : "no"; })
			<= test3;

	/* const Parser<string> test5 = apply<bool, string>(test4, test3); */
	const Parser<string> test5 = test4 ^ test3;

	/* const Parser<int> test6 = apply_first<int, string>(test1, test5); */
	const Parser<int> test6 = test1 << test5;

	/* const Parser<string> test7 = apply_second<int, string>(test6, test5); */
	const Parser<string> test7 = test6 >> test5;

	const Parser<string> test8 =
		test7 & function<string(string)>([](string x) { return ">"+x+"<"; });

	using T = string;
	const variant<ParsingError, T> result = parse<T>(test8, "foobar");
	return debug_a_test(result);
}

int test_composition_of_simple_parsers()
{
	using T = string;
	const variant<ParsingError, T> result = parse<T>(
		function<function<function<string(string)>(char)>(char)>(
			[](char a) { return [=](char b) { return [=](string c) {
				return char_as_str(a) + char_as_str(b) + "|" + c;
			}; }; }
		)
			^ any_char()
			^ parse_char('o')
			^ parse_string("obar")
			<< end_of_input(),
		"foobar"
	);
	return debug_a_test(result);
}

// }}}1
