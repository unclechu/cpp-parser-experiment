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


// Free JSON value representation type {{{1

struct JsonValue; // Algebraic data type

// Constructors-ish
struct Object: tuple<map<string, JsonValue>> {};
struct Array: tuple<list<unique_ptr<JsonValue>>> {};
struct String: tuple<string> {};
struct Number: tuple<variant<int, double>> {};
struct Bool: tuple<bool> {};
struct Null: tuple<> {};

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

	if (auto x = get_if<ParsingError>(&result)) {
		cerr << "Failed to parse: " << *x << endl;
		return EXIT_FAILURE;
	} else {
		cout << "Success: " << get<T>(result) << endl;
		return EXIT_SUCCESS;
	}
}
