#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "parser/types.hpp"

#include "abstractions/alternative.hpp"
#include "abstractions/applicative.hpp"
#include "abstractions/functor.hpp"
#include "abstractions/monadfail.hpp"

#include "parser/parsers.hpp"
#include "parser/resolvers.hpp"

#include "helpers.hpp"
#include "test.hpp"

using namespace std;

using I = ParserInputType<Parser>;


// Testing helpers {{{1

template <typename T>
ostream& operator<<(ostream &out, optional<T> &x)
{
	return x.has_value() ? out << x.value() : out << "nullopt";
}

template <typename T>
ostream& operator<<(ostream &out, ParsingResult<T, ParserInputType<Parser>> &x)
{
	using I = ParserInputType<Parser>;
	return visit(overloaded {
		[&](ParsingError<I> err) -> ostream& {
			return out
				<< "ParsingError{message=‘" << err.first
				<< "’, tail=‘" << err.second << "’}";
		},
		[&](ParsingSuccess<T, I> success_value) -> ostream& {
			return out
				<< "ParsingSuccess{value=‘" << success_value.first
				<< "’, tail=‘" << success_value.second << "’}";
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

// }}}1

class Test;
void test_basic_boilerplate(shared_ptr<Test> test);
void test_simple_parsers(shared_ptr<Test> test);
void test_composition_of_simple_parsers(shared_ptr<Test> test);

int run_test_cases()
{
	const shared_ptr<Test> test = make_shared<Test>();
	test_basic_boilerplate(test);
	test_simple_parsers(test);
	test_composition_of_simple_parsers(test);
	return test->resolve() ? EXIT_SUCCESS : EXIT_FAILURE;
}

template <typename A>
inline Parser<A> simple_parsing_failure(Parser<A> parser)
{
	return map_parsing_failure(
		[](ParsingError<I> err) { err.first = "failure"; return err; },
		parser
	);
}

void test_basic_boilerplate(shared_ptr<Test> test)
{
	const Parser<int> test_pure = pure(123);
	test->should_be<ParsingResult<int, I>>(
		"‘pure’ does not parse anything and returns the provided value",
		test_pure("foo"),
		make_parsing_success<int, I>(123, "foo")
	);

	const Parser<string> test_fmap = [&](){
		const function<string(int)> map_fn = [](int x) { return to_string(x); };

		const Parser<string> test_fn = fmap<int, string>(map_fn, test_pure);
		test->should_be<ParsingResult<string, I>>(
			"‘fmap’ (int to string conversion)",
			test_fn("foo"),
			make_parsing_success<string, I>("123", "foo")
		);

		const Parser<string> test_op = map_fn ^ test_pure;
		test->should_be<ParsingResult<string, I>>(
			"Operator ‘^’ for ‘fmap’ (int to string conversion)",
			test_op("foo"),
			make_parsing_success<string, I>("123", "foo")
		);

		return test_op;
	}();

	const Parser<bool> test_void_left = [&](){
		const Parser<bool> test_fn =
			void_left<Parser, string, bool>(test_fmap, true);
		test->should_be<ParsingResult<bool, I>>(
			"‘void_left’ (voiding string, replacing with bool)",
			test_fn("foo"),
			make_parsing_success<bool, I>(true, "foo")
		);

		const Parser<bool> test_op = test_fmap >= true;
		test->should_be<ParsingResult<bool, I>>(
			string("Operator ‘>=’ for ‘void_left’ ")
				+ "(voiding string, replacing with bool)",
			test_op("foo"),
			make_parsing_success<bool, I>(true, "foo")
		);

		return test_op;
	}();

	const Parser<function<string(bool)>> test_void_right = [&](){
		using T = function<string(bool)>;
		T f = [](bool x) { return x ? "yes" : "no"; };

		// Only testing that template compiles, see the following “test_apply”
		const Parser<T> test_fn = void_right<Parser, T, bool>(f, test_void_left);
		const Parser<T> test_op = f <= test_void_left;

		return test_op;
	}();

	const Parser<string> test_apply = [&](){
		const Parser<string> test_fn =
			apply<bool, string>(test_void_right, test_void_left);
		test->should_be<ParsingResult<string, I>>(
			"‘apply’ (applying wrapped function on wrapped argument)",
			test_fn("foo"),
			make_parsing_success<string, I>("yes", "foo")
		);

		const Parser<string> test_op = test_void_right ^ test_void_left;
		test->should_be<ParsingResult<string, I>>(
			string("Operator ‘^’ for ‘apply’ ")
				+ "(applying wrapped function on wrapped argument)",
			test_op("foo"),
			make_parsing_success<string, I>("yes", "foo")
		);

		return test_op;
	}();

	const Parser<int> test_apply_first = [&](){
		const Parser<int> test_fn =
			apply_first<Parser, int, string>(test_pure, test_apply);
		test->should_be<ParsingResult<int, I>>(
			"‘apply_first’ (discard right value, keep left one)",
			test_fn("foo"),
			make_parsing_success<int, I>(123, "foo")
		);

		const Parser<int> test_op = test_pure << test_apply;
		test->should_be<ParsingResult<int, I>>(
			"Operator ‘<<’ for ‘apply_first’ (discard right value, keep left one)",
			test_op("foo"),
			make_parsing_success<int, I>(123, "foo")
		);

		return test_op;
	}();

	const Parser<string> test_apply_second = [&](){
		const Parser<string> test_fn =
			apply_second<Parser, int, string>(test_apply_first, test_apply);
		test->should_be<ParsingResult<string, I>>(
			"‘apply_second’ (discard left value, keep right one)",
			test_fn("foo"),
			make_parsing_success<string, I>("yes", "foo")
		);

		const Parser<string> test_op = test_apply_first >> test_apply;
		test->should_be<ParsingResult<string, I>>(
			"‘apply_second’ (discard left value, keep right one)",
			test_op("foo"),
			make_parsing_success<string, I>("yes", "foo")
		);

		return test_op;
	}();

	const Parser<string> test_flipped_fmap =
		test_apply_second
			& function<string(string)>([](string x) { return ">"+x+"<"; });
	test->should_be<ParsingResult<string, I>>(
		"‘&’ (flipped fmap operator) maps the value",
		test_flipped_fmap("foo"),
		make_parsing_success<string, I>(">yes<", "foo")
	);

	const variant<ParsingError<I>, string> result =
		parse<string, Parser>(test_flipped_fmap, "foobar");
	test->should_be<string>(
		"‘parse’ returns correct result",
		visit(overloaded {
			[](ParsingError<I> err) { return "ParsingError{" + err.first + "}"; },
			[](string x) { return x; }
		}, result),
		">yes<"
	);

	// Fail parser {{{3
	{
		const Parser<int> test_fail =
			fail("Failed to parse") >> test_apply_first;
		test->should_be<ParsingResult<int, I>>(
			"‘fail’ fails the parser",
			test_fail("foo"),
			make_parsing_error<I>("Failed to parse", "foo")
		);
	}{
		const Parser<int> test_fail =
			test_apply_first << fail("Failed to parse");
		test->should_be<ParsingResult<int, I>>(
			"‘fail’ fails the parser no matter in what order it’s composed",
			test_fail("foo"),
			make_parsing_error<I>("Failed to parse", "foo")
		);
	}{
		const Parser<int> test_fail = 123 <= fail("Failed to parse");
		test->should_be<ParsingResult<int, I>>(
			"‘fail’ fails with ‘<=’ operator (‘void_right’)",
			test_fail("foo"),
			make_parsing_error<I>("Failed to parse", "foo")
		);
	}{
		const Parser<int> test_fail = fail("Failed to parse") >= 123;
		test->should_be<ParsingResult<int, I>>(
			"‘fail’ fails with ‘<=’ operator (‘void_left’)",
			test_fail("foo"),
			make_parsing_error<I>("Failed to parse", "foo")
		);
	}{
		const Parser<int> test_fail = fail<int>("Failed to parse");
		test->should_be<ParsingResult<int, I>>(
			"‘fail’ fails alone having any type",
			test_fail("foo"),
			make_parsing_error<I>("Failed to parse", "foo")
		);
	}
	// }}}3

	// Alternative {{{3
	{ // “alt” {{{4
		const Parser<int> test_two_pure = alt(pure(10), pure(20));
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns first when have two successful values",
			test_two_pure("foo"),
			make_parsing_success<int, I>(10, "foo")
		);
		const Parser<int> test_two_pure_op = pure(10) || pure(20);
		test->should_be<ParsingResult<int, I>>(
			"‘||’ works the same way as ‘alt’",
			test_two_pure_op("foo"),
			make_parsing_success<int, I>(10, "foo")
		);

		const Parser<int> test_second_pure =
			fail<int>("failure") || pure(20);
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns second when first is a failure",
			test_second_pure("foo"),
			make_parsing_success<int, I>(20, "foo")
		);

		const Parser<int> test_both_failed =
			fail<int>("one") || fail<int>("two");
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns last error if both have failed",
			test_both_failed("foo"),
			make_parsing_error<I>("two", "foo")
		);

		const Parser<int> test_three_failed =
			fail<int>("one")
				|| fail<int>("two")
				|| fail<int>("three");
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns last error if three have failed",
			test_three_failed("foo"),
			make_parsing_error<I>("three", "foo")
		);

		const Parser<int> test_three_second_successful =
			fail<int>("one")
				|| pure(20)
				|| fail<int>("three");
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns second successful when 2 other failed",
			test_three_second_successful("foo"),
			make_parsing_success<int, I>(20, "foo")
		);

		const Parser<int> test_three_last_successful =
			fail<int>("one")
				|| fail<int>("two")
				|| pure(30);
		test->should_be<ParsingResult<int, I>>(
			"‘alt’ returns last successful when 2 other failed",
			test_three_last_successful("foo"),
			make_parsing_success<int, I>(30, "foo")
		);
	} // }}}4

	function<string(vector<string>)> debug_list_of_strings =
		[](vector<string> list) -> string {
			if (list.empty()) return "0:list_is_empty";
			ostringstream out;
			out << list.size() << ":";
			ostream_iterator<string> out_iterator (out, ",");
			copy(list.begin(), list.end()-1, out_iterator);
			out << list.back(); // last element without separator
			return out.str();
		};

	const Parser<string> foobarbaz_parser =
		char_('(')
		>> (string_("foo") || string_("bar") || string_("baz"))
		<< char_ (')');

	{ // “some” {{{4
		const Parser<vector<string>> test_3_elems =
			some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘some’ parses ‘(foo)(bar)(baz) as 3 elements’",
			(debug_list_of_strings ^ test_3_elems)("(foo)(bar)(baz)tail"),
			make_parsing_success<string, I>("3:foo,bar,baz", "tail")
		);

		const Parser<vector<string>> test_3_elems_in_reverse =
			some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘some’ parses ‘(baz)(bar)(foo)’ as 3 elements",
			(debug_list_of_strings ^ test_3_elems_in_reverse)("(baz)(bar)(foo)tail"),
			make_parsing_success<string, I>("3:baz,bar,foo", "tail")
		);

		const Parser<vector<string>> test_1_elem = some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘some’ parses ‘(bar)’ as 1 element",
			(debug_list_of_strings ^ test_1_elem)("(bar)tail"),
			make_parsing_success<string, I>("1:bar", "tail")
		);

		const Parser<vector<string>> test_failure =
			simple_parsing_failure(some<string>(foobarbaz_parser));
		test->should_be<ParsingResult<string, I>>(
			"‘some’ ensures that at least one element is parsed",
			(debug_list_of_strings ^ test_failure)("foobar"),
			make_parsing_error<I>("failure", "foobar")
		);
	} // }}}4
	{ // “many” {{{4
		const Parser<vector<string>> test_3_elems =
			many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘many’ parses ‘(foo)(bar)(baz) as 3 elements’",
			(debug_list_of_strings ^ test_3_elems)("(foo)(bar)(baz)tail"),
			make_parsing_success<string, I>("3:foo,bar,baz", "tail")
		);

		const Parser<vector<string>> test_3_elems_in_reverse =
			many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘many’ parses ‘(baz)(bar)(foo)’ as 3 elements",
			(debug_list_of_strings ^ test_3_elems_in_reverse)("(baz)(bar)(foo)tail"),
			make_parsing_success<string, I>("3:baz,bar,foo", "tail")
		);

		const Parser<vector<string>> test_1_elem = many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string, I>>(
			"‘many’ parses ‘(bar)’ as 1 element",
			(debug_list_of_strings ^ test_1_elem)("(bar)tail"),
			make_parsing_success<string, I>("1:bar", "tail")
		);

		const Parser<vector<string>> test_failure =
			simple_parsing_failure(many<string>(foobarbaz_parser));
		test->should_be<ParsingResult<string, I>>(
			"‘many’ is okay with parsing nothing (empty list)",
			(debug_list_of_strings ^ test_failure)("foobar"),
			make_parsing_success<string, I>("0:list_is_empty", "foobar")
		);
	} // }}}4
	{ // “optional_parser” {{{4
		const Parser<optional<char>> test_optional_x =
			optional_parser(char_('x'));
		test->should_be<ParsingResult<optional<char>, I>>(
			"‘optional_parser’ of ‘x’ char parses the char",
			test_optional_x("xyz"),
			make_parsing_success<optional<char>, I>('x', "yz")
		);
		test->should_be<ParsingResult<optional<char>, I>>(
			"‘optional_parser’ of ‘x’ resolves to ‘nullopt’",
			test_optional_x("foo"),
			make_parsing_success<optional<char>, I>(nullopt, "foo")
		);
	} // }}}4
	// }}}3
}

template <typename T>
void generic_decimal_parser_test(
	string fn_name,
	Parser<T> test_parser,
	shared_ptr<Test> test
)
{
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘123’ as an integer",
		test_parser("123tail"),
		make_parsing_success<T, I>(123, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘987’ as an integer",
		test_parser("987tail"),
		make_parsing_success<T, I>(987, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘1’ as an integer",
		test_parser("1tail"),
		make_parsing_success<T, I>(1, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘000123’ as ‘123’ integer",
		test_parser("000123tail"),
		make_parsing_success<T, I>(123, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘0’ as an integer",
		test_parser("0tail"),
		make_parsing_success<T, I>(0, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ parses ‘000’ as ‘0’ integer",
		test_parser("000tail"),
		make_parsing_success<T, I>(0, "tail")
	);
	test->should_be<ParsingResult<T, I>>(
		"‘" + fn_name + "’ fails when number overflows",
		simple_parsing_failure(test_parser)(
			"99999999999999999999999999999999999999999999999999tail"
		),
		make_parsing_error<I>(
			"failure",
			"99999999999999999999999999999999999999999999999999tail"
		)
	);
}

void generic_fractional_parser_test(
	string fn_name,
	Parser<double> test_parser,
	shared_ptr<Test> test
)
{
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ parses ‘123.456’",
		test_parser("123.456tail"),
		make_parsing_success<double, I>(123.456, "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ parses ‘1.0’",
		test_parser("1.0tail"),
		make_parsing_success<double, I>(1.0, "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ parses ‘0.0’",
		test_parser("0.0tail"),
		make_parsing_success<double, I>(0.0, "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ parses ‘000123.000321’ as ‘123.000321’",
		test_parser("000123.000321tail"),
		make_parsing_success<double, I>(123.000321, "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ parses ‘000123.321000’ as ‘123.321’",
		test_parser("000123.321000tail"),
		make_parsing_success<double, I>(123.321, "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ fails to parse ‘1’ (without dot)",
		simple_parsing_failure(test_parser)("1tail"),
		make_parsing_error<I>("failure", "tail")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (1)",
		simple_parsing_failure(test_parser)("1.x"),
		make_parsing_error<I>("failure", "x")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (2)",
		simple_parsing_failure(test_parser)("x.1"),
		make_parsing_error<I>("failure", "x.1")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (3)",
		simple_parsing_failure(test_parser)("xyz"),
		make_parsing_error<I>("failure", "xyz")
	);
	test->should_be<ParsingResult<double, I>>(
		"‘" + fn_name + "’ fails to parse on empty input",
		simple_parsing_failure(test_parser)(""),
		make_parsing_error<I>("failure", "")
	);
	{
		string test_input =
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string("99999999999999999999999999999999999999999999999999") +
			string(".9tail");
		test->should_be<ParsingResult<double, I>>(
			"‘" + fn_name + "’ fails when number overflows",
			simple_parsing_failure(test_parser)(test_input),
			make_parsing_error<I>("failure", test_input)
		);
	}
}

void test_simple_parsers(shared_ptr<Test> test)
{
	{ // end_of_input {{{2
		test->should_be<ParsingResult<Unit, I>>(
			"‘end_of_input’ succeeds on empty input",
			end_of_input()(""),
			make_parsing_success<Unit, I>(unit(), "")
		);
	} // }}}2
	{ // char parsers {{{2
		{ // any_char {{{3
			test->should_be<ParsingResult<char, I>>(
				"‘any_char’ parses any char (1)",
				any_char()("foo"),
				make_parsing_success<char, I>('f', "oo")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘any_char’ parses any char (2)",
				any_char()("x"),
				make_parsing_success<char, I>('x', "")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘any_char’ fails on empty input",
				simple_parsing_failure(any_char())(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // char_ {{{3
			test->should_be<ParsingResult<char, I>>(
				"‘char_’ parses specified char",
				char_('f')("foo"),
				make_parsing_success<char, I>('f', "oo")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘char_’ fails to parse if char is different",
				simple_parsing_failure(char_('f'))("bar"),
				make_parsing_error<I>("failure", "bar")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘char_’ fails on empty input",
				simple_parsing_failure(char_('f'))(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // not_char {{{3
			test->should_be<ParsingResult<char, I>>(
				"‘not_char’ parses char that is not equal to provided one",
				not_char('f')("bar"),
				make_parsing_success<char, I>('b', "ar")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘not_char’ fails to parse if char is the same",
				simple_parsing_failure(not_char('f'))("foo"),
				make_parsing_error<I>("failure", "foo")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘not_char’ fails on empty input",
				simple_parsing_failure(not_char('f'))(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // satisfy {{{3
			const function<bool(char)> predicate = [](char x) {
				return x == 'f';
			};
			test->should_be<ParsingResult<char, I>>(
				"‘satisfy’ parses char that satisfies predicate",
				satisfy(predicate)("foo"),
				make_parsing_success<char, I>('f', "oo")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘satisfy’ fails to parse if char dissatisfies predicate",
				simple_parsing_failure(satisfy(predicate))("bar"),
				make_parsing_error<I>("failure", "bar")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘satisfy’ fails on empty input",
				simple_parsing_failure(satisfy(predicate))(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // digit {{{3
			test->should_be<ParsingResult<char, I>>(
				"‘digit’ parses ‘0’",
				digit()("0tail"),
				make_parsing_success<char, I>('0', "tail")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘digit’ parses ‘1’",
				digit()("1tail"),
				make_parsing_success<char, I>('1', "tail")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘digit’ parses ‘9’",
				digit()("9tail"),
				make_parsing_success<char, I>('9', "tail")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘digit’ fails to parse if it’s not a digit",
				simple_parsing_failure(digit())("foo"),
				make_parsing_error<I>("failure", "foo")
			);
			test->should_be<ParsingResult<char, I>>(
				"‘digit’ fails on empty input",
				simple_parsing_failure(digit())(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // num_sign {{{3
			const function<int(function<int(int)>)> apply_num =
				[](function<int(int)> fn) { return fn(123); };
			test->should_be<ParsingResult<int, I>>(
				"‘num_sign’ parses ‘+’",
				(apply_num ^ num_sign<int>())("+tail"),
				make_parsing_success<int, I>(123, "tail")
			);
			test->should_be<ParsingResult<int, I>>(
				"‘num_sign’ parses ‘-’ (negates the value)",
				(apply_num ^ num_sign<int>())("-tail"),
				make_parsing_success<int, I>(-123, "tail")
			);
			test->should_be<ParsingResult<int, I>>(
				"‘num_sign’ fails to parse if neither of those signs",
				simple_parsing_failure(apply_num ^ num_sign<int>())("foo"),
				make_parsing_error<I>("failure", "foo")
			);
			test->should_be<ParsingResult<int, I>>(
				"‘num_sign’ fails on empty input",
				simple_parsing_failure(apply_num ^ num_sign<int>())(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
	} // }}}2
	{ // string parsers {{{2
		{ // string_ {{{3
			test->should_be<ParsingResult<string, I>>(
				"‘string_’ parses specified string",
				string_("foobar")("foobarbaz"),
				make_parsing_success<string, I>("foobar", "baz")
			);
			test->should_be<ParsingResult<string, I>>(
				"‘string_’ fails to parse if string is different",
				simple_parsing_failure(string_("foobar"))("xyzabcdef"),
				make_parsing_error<I>("failure", "xyzabcdef")
			);
			test->should_be<ParsingResult<string, I>>(
				"‘string_’ fails to parse if not enough input",
				simple_parsing_failure(string_("foobar"))("foo"),
				make_parsing_error<I>("failure", "foo")
			);
			test->should_be<ParsingResult<string, I>>(
				"‘string_’ fails on empty input",
				simple_parsing_failure(string_("foobar"))(""),
				make_parsing_error<I>("failure", "")
			);
		} // }}}3
		{ // digits {{{3
			test->should_be<ParsingResult<string, I>>(
				"‘digits’ parses multiple digits",
				digits()("0123456789foo"),
				make_parsing_success<string, I>("0123456789", "foo")
			);
			test->should_be<ParsingResult<string, I>>(
				"‘digits’ parses single digit",
				digits()("1foo"),
				make_parsing_success<string, I>("1", "foo")
			);
			test->should_be<ParsingResult<string, I>>(
				"‘digits’ fails to parse if there are no digits",
				simple_parsing_failure(digits())("foo"),
				make_parsing_error<I>("failure", "foo")
			);
		} // }}}3
	} // }}}2
	{ // unsigned_decimal {{{3
		const Parser<unsigned int> test_parser = unsigned_decimal();
		generic_decimal_parser_test("unsigned_decimal", test_parser, test);
		test->should_be<ParsingResult<unsigned int, I>>(
			"‘unsigned_decimal’ fails to parse if a char is not a digit",
			simple_parsing_failure(test_parser)("tail"),
			make_parsing_error<I>("failure", "tail")
		);
		test->should_be<ParsingResult<unsigned int, I>>(
			"‘unsigned_decimal’ fails to parse signed decimal",
			simple_parsing_failure(test_parser)("-1tail"),
			make_parsing_error<I>("failure", "-1tail")
		);
	} // }}}3
	{ // signed_decimal {{{3
		const Parser<int> test_parser = signed_decimal();
		generic_decimal_parser_test("signed_decimal", test_parser, test);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ fails to parse if a char is not a digit (1)",
			simple_parsing_failure(test_parser)("-tail"),
			make_parsing_error<I>("failure", "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ fails to parse if a char is not a digit (2)",
			simple_parsing_failure(test_parser)("+tail"),
			make_parsing_error<I>("failure", "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ parses ‘-1’",
			test_parser("-1tail"),
			make_parsing_success<int, I>(-1, "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ parses ‘+1’",
			test_parser("+1tail"),
			make_parsing_success<int, I>(1, "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ parses ‘-0’",
			test_parser("-0tail"),
			make_parsing_success<int, I>(0, "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ parses ‘+0’",
			test_parser("+0tail"),
			make_parsing_success<int, I>(0, "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ parses ‘-1234567890’",
			test_parser("-1234567890tail"),
			make_parsing_success<int, I>(-1234567890, "tail")
		);
		test->should_be<ParsingResult<int, I>>(
			"‘signed_decimal’ fails when negative number underflows",
			simple_parsing_failure(test_parser)(
				"-99999999999999999999999999999999999999999999999999tail"
			),
			make_parsing_error<I>(
				"failure",
				"99999999999999999999999999999999999999999999999999tail"
			)
		);
	} // }}}3
	{ // unsigned_fractional {{{3
		const Parser<double> test_parser = unsigned_fractional();
		generic_fractional_parser_test(
			"unsigned_fractional",
			test_parser,
			test
		);
		test->should_be<ParsingResult<double, I>>(
			"‘unsigned_fractional’ fails to parse signed value ‘-123.123’",
			simple_parsing_failure(test_parser)("-123.123tail"),
			make_parsing_error<I>("failure", "-123.123tail")
		);
	} // }}}3
	{ // signed_fractional {{{3
		const Parser<double> test_parser = signed_fractional();
		generic_fractional_parser_test("signed_fractional", test_parser, test);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ parses signed value ‘-123.123’",
			test_parser("-123.123tail"),
			make_parsing_success<double, I>(-123.123, "tail")
		);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ parses signed value ‘-123.123’",
			test_parser("-123.123tail"),
			make_parsing_success<double, I>(-123.123, "tail")
		);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ parses signed value ‘-0.0’",
			test_parser("-0.0tail"),
			make_parsing_success<double, I>(0.0, "tail")
		);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ parses signed value ‘+0.0’",
			test_parser("+0.0tail"),
			make_parsing_success<double, I>(0.0, "tail")
		);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ parses signed value ‘-1234567890.0’",
			test_parser("-1234567890.0tail"),
			make_parsing_success<double, I>(-1234567890.0, "tail")
		);
		test->should_be<ParsingResult<double, I>>(
			"‘signed_fractional’ fails to parse signed non-fractional ‘-1’ (no dot)",
			simple_parsing_failure(test_parser)("-1tail"),
			make_parsing_error<I>("failure", "tail")
		);
		{
			string test_input =
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string("99999999999999999999999999999999999999999999999999") +
				string(".9tail");
			test->should_be<ParsingResult<double, I>>(
				"‘signed_fractional’ fails when negative number underflows",
				simple_parsing_failure(test_parser)("-" + test_input),
				make_parsing_error<I>("failure", test_input)
			);
		}
	} // }}}3
}

void test_composition_of_simple_parsers(shared_ptr<Test> test)
{
	{
		const function<string(char, char, string)> map_fn =
			[](char a, char b, string c) {
				return char_as_str(a) + char_as_str(b) + "|" + c;
			};
		const Parser<string> test_parser =
			curry(map_fn)
			^ any_char()
			^ char_('o')
			^ string_("obar")
			<< end_of_input();
		test->should_be<ParsingResult<string, I>>(
			"‘foobar’ is fully parsed",
			test_parser("foobar"),
			make_parsing_success<string, I>("fo|obar", "")
		);
	}
}
