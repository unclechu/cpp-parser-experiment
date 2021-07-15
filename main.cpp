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

struct Unit: tuple<> {};


// Free JSON value representation type {{{1

struct JsonValue; // Algebraic data type

// Constructors-ish
struct Object: tuple<map<string, JsonValue>> {};
struct Array: tuple<list<unique_ptr<JsonValue>>> {};
struct String: tuple<string> {};
struct Number: tuple<variant<int, double>> {};
struct Bool: tuple<bool> {};
struct Null: Unit {};

struct JsonValue: variant<Object, Array, String, Number, Bool, Null> {};

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
Parser<B> fmapParser(function<B(A)> mapFn, Parser<A> parser)
{
	return Parser<B>{[=](Input input) {
		auto parsingResult = parser(input);
		ParsingResult<B> result;

		if (auto x = get_if<ParsingSuccess<A>>(&parsingResult)) {
			auto [ value, tail ] = *x;
			result = make_tuple(mapFn(value), tail);
		} else {
			result = get<ParsingError>(parsingResult);
		}

		return result;
	}};
}
template <typename A, typename B>
// Operator equivalent for “fmapParser”
Parser<B> operator|(function<B(A)> mapFn, Parser<A> parser)
{
	return fmapParser<A, B>(mapFn, parser);
}
template <typename A, typename B>
// Flipped version of “fmap” (like (<&>) comparing to (<$>))
Parser<B> operator&(Parser<A> parser, function<B(A)> mapFn)
{
	return fmapParser<A, B>(mapFn, parser);
}

template <typename A, typename B>
// (<$) :: a → Parser b → Parser a
Parser<A> voidRight(A toValue, Parser<B> parser)
{
	return fmapParser<B, A>([=](B) { return toValue; }, parser);
}
template <typename A, typename B>
// Operator equivalent for “voidRight”
Parser<A> operator<=(A toValue, Parser<B> parser)
{
	return voidRight<A, B>(toValue, parser);
}

template <typename A, typename B>
// ($>) :: Parser a → b → Parser b
Parser<B> voidLeft(Parser<A> parser, B toValue)
{
	return voidRight(toValue, parser);
}
template <typename A, typename B>
// Operator equivalent for “voidLeft”
Parser<B> operator>=(Parser<A> parser, B toValue)
{
	return voidLeft<A, B>(parser, toValue);
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
Parser<B> apply(Parser<function<B(A)>> fnParser, Parser<A> parser)
{
	return Parser<B>{[=](Input input) {
		auto parsingResult = fnParser(input);

		if (auto err = get_if<ParsingError>(&parsingResult))
			return static_cast<ParsingResult<B>>(*err);

		auto [ fn, tail ] = get<ParsingSuccess<function<B(A)>>>(parsingResult);
		return fmapParser<A, B>(fn, parser)(tail);
	}};
}
template <typename A, typename B>
// Operator equivalent for “apply”
Parser<B> operator^(Parser<function<B(A)>> fnParser, Parser<A> parser)
{
	return apply<A, B>(fnParser, parser);
}

template <typename A, typename B>
// (<*) :: Parser a → Parser b → Parser a
Parser<A> applyFirst(Parser<A> parserA, Parser<B> parserB)
{
	// (\a _ -> a) <$> parserA <*> parserB
	return apply<B, A>(
		fmapParser<A, function<A(B)>>(
			[](A a) { return [=](B) { return a; }; },
			parserA
		),
		parserB
	);
}
template <typename A, typename B>
// Operator equivalent for “applyFirst”
Parser<A> operator<<(Parser<A> parserA, Parser<B> parserB)
{
	return applyFirst<A, B>(parserA, parserB);
}

template <typename A, typename B>
// (*>) :: Parser a → Parser b → Parser b
Parser<B> applySecond(Parser<A> parserA, Parser<B> parserB)
{
	// (\_ b -> b) <$> parserA <*> parserB
	return apply<B, B>(
		fmapParser<A, function<B(B)>>(
			[](A) { return [](B b) { return b; }; },
			parserA
		),
		parserB
	);
}
template <typename A, typename B>
// Operator equivalent for “applySecond”
Parser<B> operator>>(Parser<A> parserA, Parser<B> parserB)
{
	return applySecond<A, B>(parserA, parserB);
}

// }}}2

// Parsing functions {{{2

template <typename A, typename B>
// either :: (a → c) → (b → c) → Either a b → c
function<B(ParsingResult<A>)> parsingResolver(
	function<B(ParsingError)> failureResolve,
	function<B(A)> successResolve
)
{
	return [=](ParsingResult<A> result) {
		if (auto x = get_if<ParsingSuccess<A>>(&result)) {
			auto [ value, _ ] = *x;
			return successResolve(value);
		} else {
			auto err = get<ParsingError>(result);
			return failureResolve(err);
		}
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
		parsingResolver<A, Result>(
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
	return Parser<Unit>{[](Input input) {
		ParsingResult<Unit> result;
		return input.empty()
			? result = make_tuple(Unit{}, input)
			: result = ParsingError{"end_of_input: input is not empty"};
	}};
}

Parser<char> any_char()
{
	return Parser<char>{[](Input input) {
		ParsingResult<char> result;
		return input.empty()
			? result = ParsingError{"any_char: input is empty"}
			: result = make_tuple(input[0], input.substr(1));
	}};
}

Parser<char> parse_char(char c)
{
	return Parser<char>{[=](Input input) {
		ParsingResult<char> result;

		auto pfx = [=](string msg){
			return "parse_char('" + char_as_str(c) + "'): " + msg;
		};

		if (input.empty())
			return result = ParsingError{pfx("input is empty")};
		else if (input[0] != c)
			return result = ParsingError{pfx(
				"char is different, got this: '" + char_as_str(input[0]) + "'"
			)};
		else
			return result = make_tuple(input[0], input.substr(1));
	}};
}

// Just takes what’s left in the input
Parser<string> any_string()
{
	return Parser<string>{[](Input input) { return make_tuple(input, ""); }};
}

Parser<string> parse_string(string s)
{
	return Parser<string>{[=](Input input) {
		ParsingResult<string> result;

		auto pfx = [=](string msg){
			return "parse_string(\"" + s + "\"): " + msg;
		};

		string taken_string;

		if (input.empty())
			return result = ParsingError{pfx("input is empty")};
		else if (input.size() < s.size())
			return result = ParsingError{pfx(
				"input is less than string (input is: \"" + input + "\")"
			)};
		else if ((taken_string = input.substr(0, s.size())) != s)
			return result = ParsingError{pfx(
				"string is different, got this: \"" + taken_string + "\""
			)};
		else
			return result = make_tuple(taken_string, input.substr(s.size()));
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

	/* const Parser<string> test2 = fmapParser<int, string>([](int x) { */
	/* 	return to_string(x); */
	/* }, test1); */
	const Parser<string> test2 =
		function<string(int)>([](int x) { return to_string(x); }) | test1;

	const Parser<bool> test3 = test2 >= true;

	/* const Parser<function<string(bool)>> test4 = */
	/* 	voidRight<function<string(bool)>, bool>( */
	/* 		[](bool x) { return x ? "yes" : "no"; }, */
	/* 		test3 */
	/* 	); */
	const Parser<function<string(bool)>> test4 =
		function<string(bool)>([](bool x) { return x ? "yes" : "no"; })
			<= test3;

	/* const Parser<string> test5 = apply<bool, string>(test4, test3); */
	const Parser<string> test5 = test4 ^ test3;

	/* const Parser<int> test6 = applyFirst<int, string>(test1, test5); */
	const Parser<int> test6 = test1 << test5;

	/* const Parser<string> test7 = applySecond<int, string>(test6, test5); */
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
		(function<function<function<string(string)>(char)>(char)>(
			[](char a) { return [=](char b) { return [=](string c) {
				return char_as_str(a) + char_as_str(b) + "|" + c;
			}; }; }
		) | any_char())
			^ parse_char('o')
			^ parse_string("obar")
			<< end_of_input(),
		"foobar"
	);
	return debug_a_test(result);
}

// }}}1
