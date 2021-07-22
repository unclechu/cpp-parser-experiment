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
ostream& operator<<(ostream &out, optional<T> &x)
{
	return x.has_value() ? out << x.value() : out << "nullopt";
}

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
	return map_parsing_failure(const_map(ParsingError{"failure"}), parser);
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

	{ // “some” {{{4
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

		const Parser<vector<string>> test_failure =
			simple_parsing_failure(some<string>(foobarbaz_parser));
		test->should_be<ParsingResult<string>>(
			"‘some’ ensures that at least one element is parsed",
			(debug_list_of_strings ^ test_failure)("foobar"),
			ParsingError{"failure"}
		);
	} // }}}4
	{ // “many” {{{4
		const Parser<vector<string>> test_3_elems =
			many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘many’ parses ‘(foo)(bar)(baz) as 3 elements’",
			(debug_list_of_strings ^ test_3_elems)("(foo)(bar)(baz)tail"),
			make_tuple("3:foo,bar,baz", "tail")
		);

		const Parser<vector<string>> test_3_elems_in_reverse =
			many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘many’ parses ‘(baz)(bar)(foo)’ as 3 elements",
			(debug_list_of_strings ^ test_3_elems_in_reverse)("(baz)(bar)(foo)tail"),
			make_tuple("3:baz,bar,foo", "tail")
		);

		const Parser<vector<string>> test_1_elem = many<string>(foobarbaz_parser);
		test->should_be<ParsingResult<string>>(
			"‘many’ parses ‘(bar)’ as 1 element",
			(debug_list_of_strings ^ test_1_elem)("(bar)tail"),
			make_tuple("1:bar", "tail")
		);

		const Parser<vector<string>> test_failure =
			simple_parsing_failure(many<string>(foobarbaz_parser));
		test->should_be<ParsingResult<string>>(
			"‘many’ is okay with parsing nothing (empty list)",
			(debug_list_of_strings ^ test_failure)("foobar"),
			make_tuple("0:list_is_empty", "foobar")
		);
	} // }}}4
	{ // “optional_parser” {{{4
		const Parser<optional<char>> test_optional_x =
			optional_parser(parse_char('x'));
		test->should_be<ParsingResult<optional<char>>>(
			"‘optional_parser’ of ‘x’ char parses the char",
			test_optional_x("xyz"),
			make_tuple('x', "yz")
		);
		test->should_be<ParsingResult<optional<char>>>(
			"‘optional_parser’ of ‘x’ resolves to ‘nullopt’",
			test_optional_x("foo"),
			make_tuple(nullopt, "foo")
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
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘123’ as an integer",
		test_parser("123tail"),
		make_tuple(123, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘987’ as an integer",
		test_parser("987tail"),
		make_tuple(987, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘1’ as an integer",
		test_parser("1tail"),
		make_tuple(1, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘000123’ as ‘123’ integer",
		test_parser("000123tail"),
		make_tuple(123, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘0’ as an integer",
		test_parser("0tail"),
		make_tuple(0, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ parses ‘000’ as ‘0’ integer",
		test_parser("000tail"),
		make_tuple(0, "tail")
	);
	test->should_be<ParsingResult<T>>(
		"‘" + fn_name + "’ fails when number overflows",
		simple_parsing_failure(test_parser)(
			"99999999999999999999999999999999999999999999999999tail"
		),
		ParsingError{"failure"}
	);
}

void generic_fractional_parser_test(
	string fn_name,
	Parser<double> test_parser,
	shared_ptr<Test> test
)
{
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ parses ‘123.456’",
		test_parser("123.456tail"),
		make_tuple(123.456, "tail")
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ parses ‘1.0’",
		test_parser("1.0tail"),
		make_tuple(1.0, "tail")
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ parses ‘0.0’",
		test_parser("0.0tail"),
		make_tuple(0.0, "tail")
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ parses ‘000123.000321’ as ‘123.000321’",
		test_parser("000123.000321tail"),
		make_tuple(123.000321, "tail")
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ parses ‘000123.321000’ as ‘123.321’",
		test_parser("000123.321000tail"),
		make_tuple(123.321, "tail")
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails to parse ‘1’ (without dot)",
		simple_parsing_failure(test_parser)("1tail"),
		ParsingError{"failure"}
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (1)",
		simple_parsing_failure(test_parser)("1.x"),
		ParsingError{"failure"}
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (2)",
		simple_parsing_failure(test_parser)("x.1"),
		ParsingError{"failure"}
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails to parse when it’s not an a digit (3)",
		simple_parsing_failure(test_parser)("xyz"),
		ParsingError{"failure"}
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails to parse on empty input",
		simple_parsing_failure(test_parser)(""),
		ParsingError{"failure"}
	);
	test->should_be<ParsingResult<double>>(
		"‘" + fn_name + "’ fails when number overflows",
		simple_parsing_failure(test_parser)(
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
			string(".9tail")
		),
		ParsingError{"failure"}
	);
}

void test_simple_parsers(shared_ptr<Test> test)
{
	{ // end_of_input {{{2
		test->should_be<ParsingResult<Unit>>(
			"‘end_of_input’ succeeds on empty input",
			end_of_input()(""),
			make_tuple(unit(), "")
		);
	} // }}}2
	{ // char parsers {{{2
		{ // any_char {{{3
			test->should_be<ParsingResult<char>>(
				"‘any_char’ parses any char (1)",
				any_char()("foo"),
				make_tuple('f', "oo")
			);
			test->should_be<ParsingResult<char>>(
				"‘any_char’ parses any char (2)",
				any_char()("x"),
				make_tuple('x', "")
			);
			test->should_be<ParsingResult<char>>(
				"‘any_char’ fails on empty input",
				simple_parsing_failure(any_char())(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // parse_char {{{3
			test->should_be<ParsingResult<char>>(
				"‘parse_char’ parses specified char",
				parse_char('f')("foo"),
				make_tuple('f', "oo")
			);
			test->should_be<ParsingResult<char>>(
				"‘parse_char’ fails to parse if char is different",
				simple_parsing_failure(parse_char('f'))("bar"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<char>>(
				"‘parse_char’ fails on empty input",
				simple_parsing_failure(parse_char('f'))(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // not_char {{{3
			test->should_be<ParsingResult<char>>(
				"‘not_char’ parses char that is not equal to provided one",
				not_char('f')("bar"),
				make_tuple('b', "ar")
			);
			test->should_be<ParsingResult<char>>(
				"‘not_char’ fails to parse if char is the same",
				simple_parsing_failure(not_char('f'))("foo"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<char>>(
				"‘not_char’ fails on empty input",
				simple_parsing_failure(not_char('f'))(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // satisfy {{{3
			const function<bool(char)> predicate = [](char x) {
				return x == 'f';
			};
			test->should_be<ParsingResult<char>>(
				"‘satisfy’ parses char that satisfies predicate",
				satisfy(predicate)("foo"),
				make_tuple('f', "oo")
			);
			test->should_be<ParsingResult<char>>(
				"‘satisfy’ fails to parse if char dissatisfies predicate",
				simple_parsing_failure(satisfy(predicate))("bar"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<char>>(
				"‘satisfy’ fails on empty input",
				simple_parsing_failure(satisfy(predicate))(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // digit {{{3
			test->should_be<ParsingResult<char>>(
				"‘digit’ parses ‘0’",
				digit()("0tail"),
				make_tuple('0', "tail")
			);
			test->should_be<ParsingResult<char>>(
				"‘digit’ parses ‘1’",
				digit()("1tail"),
				make_tuple('1', "tail")
			);
			test->should_be<ParsingResult<char>>(
				"‘digit’ parses ‘9’",
				digit()("9tail"),
				make_tuple('9', "tail")
			);
			test->should_be<ParsingResult<char>>(
				"‘digit’ fails to parse if it’s not a digit",
				simple_parsing_failure(digit())("foo"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<char>>(
				"‘digit’ fails on empty input",
				simple_parsing_failure(digit())(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // num_sign {{{3
			const function<int(function<int(int)>)> apply_num =
				[](function<int(int)> fn) { return fn(123); };
			test->should_be<ParsingResult<int>>(
				"‘num_sign’ parses ‘+’",
				(apply_num ^ num_sign<int>())("+tail"),
				make_tuple(123, "tail")
			);
			test->should_be<ParsingResult<int>>(
				"‘num_sign’ parses ‘-’ (negates the value)",
				(apply_num ^ num_sign<int>())("-tail"),
				make_tuple(-123, "tail")
			);
			test->should_be<ParsingResult<int>>(
				"‘num_sign’ fails to parse if neither of those signs",
				simple_parsing_failure(apply_num ^ num_sign<int>())("foo"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<int>>(
				"‘num_sign’ fails on empty input",
				simple_parsing_failure(apply_num ^ num_sign<int>())(""),
				ParsingError{"failure"}
			);
		} // }}}3
	} // }}}2
	{ // string parsers {{{2
		{ // parse_string {{{3
			test->should_be<ParsingResult<string>>(
				"‘parse_string’ parses specified string",
				parse_string("foobar")("foobarbaz"),
				make_tuple("foobar", "baz")
			);
			test->should_be<ParsingResult<string>>(
				"‘parse_string’ fails to parse if string is different",
				simple_parsing_failure(parse_string("foobar"))("xyzabcdef"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<string>>(
				"‘parse_string’ fails to parse if not enough input",
				simple_parsing_failure(parse_string("foobar"))("foo"),
				ParsingError{"failure"}
			);
			test->should_be<ParsingResult<string>>(
				"‘parse_string’ fails on empty input",
				simple_parsing_failure(parse_string("foobar"))(""),
				ParsingError{"failure"}
			);
		} // }}}3
		{ // digits {{{3
			test->should_be<ParsingResult<string>>(
				"‘digits’ parses multiple digits",
				digits()("0123456789foo"),
				make_tuple("0123456789", "foo")
			);
			test->should_be<ParsingResult<string>>(
				"‘digits’ parses single digit",
				digits()("1foo"),
				make_tuple("1", "foo")
			);
			test->should_be<ParsingResult<string>>(
				"‘digits’ fails to parse if there are no digits",
				simple_parsing_failure(digits())("foo"),
				ParsingError{"failure"}
			);
		} // }}}3
	} // }}}2
	{ // unsigned_decimal {{{3
		const Parser<unsigned int> test_parser = unsigned_decimal();
		generic_decimal_parser_test("unsigned_decimal", test_parser, test);
		test->should_be<ParsingResult<unsigned int>>(
			"‘unsigned_decimal’ fails to parse if a char is not a digit",
			simple_parsing_failure(test_parser)("tail"),
			ParsingError{"failure"}
		);
		test->should_be<ParsingResult<unsigned int>>(
			"‘unsigned_decimal’ fails to parse signed decimal",
			simple_parsing_failure(test_parser)("-1tail"),
			ParsingError{"failure"}
		);
	} // }}}3
	{ // signed_decimal {{{3
		const Parser<int> test_parser = signed_decimal();
		generic_decimal_parser_test("signed_decimal", test_parser, test);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ fails to parse if a char is not a digit (1)",
			simple_parsing_failure(test_parser)("-tail"),
			ParsingError{"failure"}
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ fails to parse if a char is not a digit (2)",
			simple_parsing_failure(test_parser)("+tail"),
			ParsingError{"failure"}
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ parses ‘-1’",
			test_parser("-1tail"),
			make_tuple(-1, "tail")
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ parses ‘+1’",
			test_parser("+1tail"),
			make_tuple(1, "tail")
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ parses ‘-0’",
			test_parser("-0tail"),
			make_tuple(0, "tail")
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ parses ‘+0’",
			test_parser("+0tail"),
			make_tuple(0, "tail")
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ parses ‘-1234567890’",
			test_parser("-1234567890tail"),
			make_tuple(-1234567890, "tail")
		);
		test->should_be<ParsingResult<int>>(
			"‘signed_decimal’ fails when negative number underflows",
			simple_parsing_failure(test_parser)(
				"-99999999999999999999999999999999999999999999999999tail"
			),
			ParsingError{"failure"}
		);
	} // }}}3
	{ // unsigned_fractional {{{3
		const Parser<double> test_parser = unsigned_fractional();
		generic_fractional_parser_test(
			"unsigned_fractional",
			test_parser,
			test
		);
		test->should_be<ParsingResult<double>>(
			"‘unsigned_fractional’ fails to parse signed value ‘-123.123’",
			simple_parsing_failure(test_parser)("-123.123tail"),
			ParsingError{"failure"}
		);
	} // }}}3
	{ // signed_fractional {{{3
		const Parser<double> test_parser = signed_fractional();
		generic_fractional_parser_test("signed_fractional", test_parser, test);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ parses signed value ‘-123.123’",
			test_parser("-123.123tail"),
			make_tuple(-123.123, "tail")
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ parses signed value ‘-123.123’",
			test_parser("-123.123tail"),
			make_tuple(-123.123, "tail")
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ parses signed value ‘-0.0’",
			test_parser("-0.0tail"),
			make_tuple(0.0, "tail")
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ parses signed value ‘+0.0’",
			test_parser("+0.0tail"),
			make_tuple(0.0, "tail")
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ parses signed value ‘-1234567890.0’",
			test_parser("-1234567890.0tail"),
			make_tuple(-1234567890.0, "tail")
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ fails to parse signed non-fractional ‘-1’ (no dot)",
			simple_parsing_failure(test_parser)("-1tail"),
			ParsingError{"failure"}
		);
		test->should_be<ParsingResult<double>>(
			"‘signed_fractional’ fails when negative number underflows",
			simple_parsing_failure(test_parser)(
				string("-99999999999999999999999999999999999999999999999999") +
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
				string(".9tail")
			),
			ParsingError{"failure"}
		);
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
			^ parse_char('o')
			^ parse_string("obar")
			<< end_of_input();
		test->should_be<ParsingResult<string>>(
			"‘foobar’ is fully parsed",
			test_parser("foobar"),
			make_tuple("fo|obar", "")
		);
	}
}
