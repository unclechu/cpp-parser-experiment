#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "helpers.hpp"
#include "parser/alternative.hpp"
#include "parser/applicative.hpp"
#include "parser/functor.hpp"
#include "parser/helpers.hpp"
#include "parser/monadfail.hpp"
#include "parser/parsers.hpp"
#include "parser/resolvers.hpp"
#include "parser/types.hpp"
#include "test.hpp"

using namespace std;


// Testing helpers {{{1

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

// }}}1

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
	{ // “alt” {{{4
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
	} // }}}4
	{ // “some” {{{4
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
			parse_char('(')
			>> (parse_string("foo") || parse_string("bar") || parse_string("baz"))
			<< parse_char (')');

		const Parser<vector<string>> test_3_elems =
			some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘some’ parses ‘(foo)(bar)(baz) as 3 elements’",
			(debug_list_of_strings ^ test_3_elems)("(foo)(bar)(baz)tail"),
			make_tuple("3:foo,bar,baz", "tail")
		);

		const Parser<vector<string>> test_3_elems_in_reverse =
			some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘some’ parses ‘(baz)(bar)(foo)’ as 3 elements",
			(debug_list_of_strings ^ test_3_elems_in_reverse)("(baz)(bar)(foo)tail"),
			make_tuple("3:baz,bar,foo", "tail")
		);

		const Parser<vector<string>> test_1_elem = some<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘some’ parses ‘(bar)’ as 1 element",
			(debug_list_of_strings ^ test_1_elem)("(bar)tail"),
			make_tuple("1:bar", "tail")
		);

		const Parser<vector<string>> test_failure = map_parsing_failure(
			[](ParsingError) { return ParsingError{"failed"}; },
			some<string>(foobarbaz_parser)
		);
		test->should_be<ParsingResult<string>>(
			"‘some’ ensures that at least one element is parsed",
			(debug_list_of_strings ^ test_failure)("foobar"),
			ParsingError{"failed"}
		);
	} // }}}4
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
