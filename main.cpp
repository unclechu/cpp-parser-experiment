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
#include <vector>

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
// pure :: Applicative f => a -> Parser a
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

// MonadFail {{{2

template <typename A = Unit>
Parser<A> fail_parser(string err)
{
	return Parser<A>{[=](Input) { return ParsingError{err}; }};
}

// }}}2

// Alternative {{{2

template <typename A>
// (<|>) :: Alternative f => f a -> f a -> f a
Parser<A> alt(Parser<A> parser_a, Parser<A> parser_b)
{
	return Parser<A>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingError) -> ParsingResult<A> { return parser_b(input); },
			[](ParsingSuccess<A> x) -> ParsingResult<A> { return x; }
		}, parser_a(input));
	}};
}
template <typename A>
// Operator equivalent for “alt”
Parser<A> operator||(Parser<A> a, Parser<A> b)
{
	return alt<A>(a, b);
}

template <typename A>
// One or more.
// some :: Alternative f => f a -> f [a]
Parser<vector<A>> some(Parser<A> parser)
{
	return Parser<vector<A>>{[=](Input input) {
		return ;
	}};
}

template <typename A>
// Zero or more.
// many :: Alternative f => f a -> f [a]
Parser<vector<A>> many(Parser<A> parser)
{
	return Parser<vector<A>>{[=](Input input) {
		return ;
	}};
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


int run_test_cases();

int main()
{
	return run_test_cases();
}


// Primitive testing {{{1

// Testing helpers {{{2

template <typename T>
ostream& operator<<(ostream &out, ParsingResult<T> &x)
{
	return visit(overloaded {
		[&](ParsingError err) -> ostream& {
			return out << "ParsingError{" << err << "}";
		},
		[&](ParsingSuccess<T> success_value) -> ostream& {
			auto [ value, input ] = success_value;
			return out
				<< "ParsingSuccess{value=‘" << value
				<< "’, tail=‘" << input << "’}";
		}
	}, x);
}

class Test
{
private:
	unsigned int done = 0;
	unsigned int failed = 0;

	static void log(
		ostream& out,
		const string title,
		const function<void(ostream&)> what_is_tested,
		const bool resolve
	)
	{
		out << title << ":" << endl << "  ";
		what_is_tested(out);
		out << endl << "  ";
		out << (resolve ? "[SUCCESS]" : "!!! [FAILURE] !!! ") << endl << endl;
	}

public:
	template <typename T>
	void should_be(const string title, T value, T should_be_this)
	{
		++this->done;

		const auto log_what = [&](ostream& out) {
			out << "“" << value << "” should be equal to “";
			out << should_be_this << "”…";
		};

		if (value == should_be_this) {
			this->log(cout, title, log_what, true);
		} else {
			++this->failed;
			this->log(cerr, title, log_what, false);
		}
	}

	// bool indicates whether all the tests were successful
	bool resolve()
	{
		if (failed > 0) {
			cerr
				<< "Executed " << done << " test(s) and "
				<< failed << " of them have failed" << endl;
			return false;
		} else {
			cout
				<< "Executed " << done
				<< " test(s) and all of them were successful" << endl;
			return true;
		}
	}
};

// }}}2

class Test;
void test_basic_boilerplate(shared_ptr<Test> test);
void test_composition_of_simple_parsers(shared_ptr<Test> test);

int run_test_cases()
{
	const shared_ptr<Test> test = make_shared<Test>();
	test_basic_boilerplate(test);
	test_composition_of_simple_parsers(test);
	return test->resolve() ? EXIT_SUCCESS : EXIT_FAILURE;
}

void test_basic_boilerplate(shared_ptr<Test> test)
{
	const Parser<int> test_pure = pure(123);
	test->should_be<ParsingResult<int>>(
		"‘pure’ does not parse anything and returns the provided value",
		test_pure("foo"),
		make_tuple(123, "foo")
	);

	const Parser<string> test_fmap = [&](){
		const function<string(int)> map_fn = [](int x) { return to_string(x); };

		const Parser<string> test_fn =
			fmap_parser<int, string>(map_fn, test_pure);
		test->should_be<ParsingResult<string>>(
			"‘fmap_parser’ (int to string conversion)",
			test_fn("foo"),
			make_tuple("123", "foo")
		);

		const Parser<string> test_op = map_fn ^ test_pure;
		test->should_be<ParsingResult<string>>(
			"Operator ‘^’ for ‘fmap_parser’ (int to string conversion)",
			test_op("foo"),
			make_tuple("123", "foo")
		);

		return test_op;
	}();

	const Parser<bool> test_void_left = [&](){
		const Parser<bool> test_fn = void_left<string, bool>(test_fmap, true);
		test->should_be<ParsingResult<bool>>(
			"‘void_left’ (voiding string, replacing with bool)",
			test_fn("foo"),
			make_tuple(true, "foo")
		);

		const Parser<bool> test_op = test_fmap >= true;
		test->should_be<ParsingResult<bool>>(
			string("Operator ‘>=’ for ‘void_left’ ")
				+ "(voiding string, replacing with bool)",
			test_op("foo"),
			make_tuple(true, "foo")
		);

		return test_op;
	}();

	const Parser<function<string(bool)>> test_void_right = [&](){
		using T = function<string(bool)>;
		T f = [](bool x) { return x ? "yes" : "no"; };

		// Only testing that template compiles, see the following “test_apply”
		const Parser<T> test_fn = void_right<T, bool>(f, test_void_left);
		const Parser<T> test_op = f <= test_void_left;

		return test_op;
	}();

	const Parser<string> test_apply = [&](){
		const Parser<string> test_fn =
			apply<bool, string>(test_void_right, test_void_left);
		test->should_be<ParsingResult<string>>(
			"‘apply’ (applying wrapped function on wrapped argument)",
			test_fn("foo"),
			make_tuple("yes", "foo")
		);

		const Parser<string> test_op = test_void_right ^ test_void_left;
		test->should_be<ParsingResult<string>>(
			string("Operator ‘^’ for ‘apply’ ")
				+ "(applying wrapped function on wrapped argument)",
			test_op("foo"),
			make_tuple("yes", "foo")
		);

		return test_op;
	}();

	const Parser<int> test_apply_first = [&](){
		const Parser<int> test_fn =
			apply_first<int, string>(test_pure, test_apply);
		test->should_be<ParsingResult<int>>(
			"‘apply_first’ (discard right value, keep left one)",
			test_fn("foo"),
			make_tuple(123, "foo")
		);

		const Parser<int> test_op = test_pure << test_apply;
		test->should_be<ParsingResult<int>>(
			"Operator ‘<<’ for ‘apply_first’ (discard right value, keep left one)",
			test_op("foo"),
			make_tuple(123, "foo")
		);

		return test_op;
	}();

	const Parser<string> test_apply_second = [&](){
		const Parser<string> test_fn =
			apply_second<int, string>(test_apply_first, test_apply);
		test->should_be<ParsingResult<string>>(
			"‘apply_second’ (discard left value, keep right one)",
			test_fn("foo"),
			make_tuple("yes", "foo")
		);

		const Parser<string> test_op = test_apply_first >> test_apply;
		test->should_be<ParsingResult<string>>(
			"‘apply_second’ (discard left value, keep right one)",
			test_op("foo"),
			make_tuple("yes", "foo")
		);

		return test_op;
	}();

	const Parser<string> test_flipped_fmap =
		test_apply_second
			& function<string(string)>([](string x) { return ">"+x+"<"; });
	test->should_be<ParsingResult<string>>(
		"‘&’ (flipped fmap operator) maps the value",
		test_flipped_fmap("foo"),
		make_tuple(">yes<", "foo")
	);

	const variant<ParsingError, string> result =
		parse<string>(test_flipped_fmap, "foobar");
	test->should_be<string>(
		"‘parse’ returns correct result",
		visit(overloaded {
			[](ParsingError err) { return "ParsingError{" + err + "}"; },
			[](string x) { return x; }
		}, result),
		">yes<"
	);

	// Fail parser {{{3
	{
		const Parser<int> test_fail =
			fail_parser("Failed to parse") >> test_apply_first;
		test->should_be<ParsingResult<int>>(
			"‘fail_parser’ fails the parser",
			test_fail("foo"),
			ParsingError{"Failed to parse"}
		);
	}{
		const Parser<int> test_fail =
			test_apply_first << fail_parser("Failed to parse");
		test->should_be<ParsingResult<int>>(
			"‘fail_parser’ fails the parser no matter in what order it’s composed",
			test_fail("foo"),
			ParsingError{"Failed to parse"}
		);
	}{
		const Parser<int> test_fail = 123 <= fail_parser("Failed to parse");
		test->should_be<ParsingResult<int>>(
			"‘fail_parser’ fails with ‘<=’ operator (‘void_right’)",
			test_fail("foo"),
			ParsingError{"Failed to parse"}
		);
	}{
		const Parser<int> test_fail = fail_parser("Failed to parse") >= 123;
		test->should_be<ParsingResult<int>>(
			"‘fail_parser’ fails with ‘<=’ operator (‘void_left’)",
			test_fail("foo"),
			ParsingError{"Failed to parse"}
		);
	}{
		const Parser<int> test_fail = fail_parser<int>("Failed to parse");
		test->should_be<ParsingResult<int>>(
			"‘fail_parser’ fails alone having any type",
			test_fail("foo"),
			ParsingError{"Failed to parse"}
		);
	}
	// }}}3

	// Alternative {{{3
	{
		const Parser<int> test_two_pure = alt(pure(10), pure(20));
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns first when have two successful values",
			test_two_pure("foo"),
			make_tuple(10, "foo")
		);
		const Parser<int> test_two_pure_op = pure(10) || pure(20);
		test->should_be<ParsingResult<int>>(
			"‘||’ works the same way as ‘alt’",
			test_two_pure_op("foo"),
			make_tuple(10, "foo")
		);

		const Parser<int> test_second_pure =
			fail_parser<int>("failure") || pure(20);
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns second when first is a failure",
			test_second_pure("foo"),
			make_tuple(20, "foo")
		);

		const Parser<int> test_both_failed =
			fail_parser<int>("one") || fail_parser<int>("two");
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns last error if both have failed",
			test_both_failed("foo"),
			ParsingError{"two"}
		);

		const Parser<int> test_three_failed =
			fail_parser<int>("one")
				|| fail_parser<int>("two")
				|| fail_parser<int>("three");
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns last error if three have failed",
			test_three_failed("foo"),
			ParsingError{"three"}
		);

		const Parser<int> test_three_second_successful =
			fail_parser<int>("one")
				|| pure(20)
				|| fail_parser<int>("three");
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns second successful when 2 other failed",
			test_three_second_successful("foo"),
			make_tuple(20, "foo")
		);

		const Parser<int> test_three_last_successful =
			fail_parser<int>("one")
				|| fail_parser<int>("two")
				|| pure(30);
		test->should_be<ParsingResult<int>>(
			"‘alt’ returns last successful when 2 other failed",
			test_three_last_successful("foo"),
			make_tuple(30, "foo")
		);
	}
	// }}}3
}

void test_composition_of_simple_parsers(shared_ptr<Test> test)
{
	{
		const ParsingResult<string> result = (
			function<function<function<string(string)>(char)>(char)>(
				[](char a) { return [=](char b) { return [=](string c) {
					return char_as_str(a) + char_as_str(b) + "|" + c;
				}; }; }
			)
				^ any_char()
				^ parse_char('o')
				^ parse_string("obar")
				<< end_of_input()
		)("foobar");

		test->should_be<ParsingResult<string>>
			("‘foobar’ is fully parsed", result, make_tuple("fo|obar", ""));
	}
}

// }}}1
