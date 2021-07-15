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


// Free JSON value representation type

struct JsonValue; // Algebraic data type

// Constructors-ish
struct Object: tuple<map<string, JsonValue>> {};
struct Array: tuple<list<unique_ptr<JsonValue>>> {};
struct String: tuple<string> {};
struct Number: tuple<variant<int, double>> {};
struct Bool: tuple<bool> {};
struct Null: tuple<> {};

struct JsonValue: variant<Object, Array, String, Number, Bool, Null> {};


// Basic parser implementation

using Input = string;
struct ParsingError: string {};

template <typename A>
using ParsingSuccess = tuple<A, Input>;

template <typename A>
using ParsingResult = variant<ParsingError, ParsingSuccess<A>>;

template <typename A>
// Parser a = String → Either String (a, String)
struct Parser: function<ParsingResult<A>(Input)> {};

// Functor-like stuff
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
// (<$) :: a → Parser b → Parser a
Parser<A> voidRight(A toValue, Parser<B> parser)
{
	return fmapParser<B, A>([=](B) { return toValue; }, parser);
}
template <typename A, typename B>
// ($>) :: Parser a → b → Parser b
Parser<B> voidLeft(Parser<A> parser, B toValue)
{
	return voidRight(toValue, parser);
}

// Applicative-like stuff
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


int test_basic_stuff();

int main()
{
	return test_basic_stuff();
}


int test_basic_stuff()
{
	/*const Parser<int> test1 = Parser<int>{[](string x) {
		// return make_tuple(123, x);
		return ParsingError{"oh no!"};
	}};*/
	const Parser<int> test1 = pure(123);

	const Parser<string> test2 = fmapParser<int, string>([](int x) {
		return to_string(x);
	}, test1);

	const Parser<bool> test3 = voidLeft<string, bool>(test2, true);

	const Parser<function<string(bool)>> test4 =
		voidRight<function<string(bool)>, bool>(
			[](bool x) { return x ? "yes" : "no"; },
			test3
		);

	const Parser<string> test5 = apply<bool, string>(test4, test3);
	const Parser<int> test6 = applyFirst<int, string>(test1, test5);
	const Parser<string> test7 = applySecond<int, string>(test6, test5);

	using T = string;
	const variant<ParsingError, T> result = parse<T>(test7, "foobar");

	if (auto x = get_if<ParsingError>(&result)) {
		cerr << "Failed to parse: " << *x << endl;
		return EXIT_FAILURE;
	} else {
		cout << "Success: " << get<T>(result) << endl;
		return EXIT_SUCCESS;
	}
}
