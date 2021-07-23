#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "abstractions/alternative.hpp"
#include "abstractions/applicative.hpp"
#include "abstractions/functor.hpp"
#include "helpers.hpp"
#include "json.hpp"
#include "parser/parsers.hpp"
#include "parser/types.hpp"

using namespace std;


// Parsers {{{1

Parser<JsonNull> json_null()
{
	return prefix_parsing_failure(
		"JsonNull",
		string_("null") >= JsonNull{unit()}
	);
}

Parser<JsonBool> json_bool()
{
	function<JsonBool(bool)> fn = [](bool x) {
		return JsonBool{make_tuple(x)};
	};
	return prefix_parsing_failure(
		"JsonBool",
		fn ^ (string_("true") >= true || string_("false") >= false)
	);
}

template <typename T>
inline JsonNumber to_json_number(T x)
{
	return JsonNumber{make_tuple(x)};
}

Parser<JsonNumber> json_number()
{
	return prefix_parsing_failure(
		"JsonNumber",
		(function(to_json_number<double>) ^ signed_fractional()) ||
		(function(to_json_number<int>) ^ signed_decimal())
	);
}

// WARNING! This implementation is incomplete. For instance escaped unicode
// characters are not supported (e.g. “\uD83D\uDE10”).
// You can find more details here: https://www.ietf.org/rfc/rfc4627.txt
Parser<JsonString> json_string()
{
	function<bool(char)> non_quote_char = [](char x) { return x != '"'; };
	function<JsonString(string)> to_json_string = [](string x) {
		return JsonString{make_tuple(x)};
	};
	return prefix_parsing_failure(
		"JsonString",
		to_json_string
		^ (
			function(chars_to_string<vector>)
			^ char_('"')
			>> some(('"' <= string_("\\\"")) || satisfy(non_quote_char))
			<< char_('"')
		)
	);
}

Parser<string> spacer()
{
	function<bool(char)> is_spacer = [](char c) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	};
	return function(chars_to_string<vector>) ^ some(satisfy(is_spacer));
}

// }}}1
