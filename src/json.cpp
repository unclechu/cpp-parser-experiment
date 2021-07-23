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

JsonString make_json_string(string x)
{
	return JsonString{make_tuple(x)};
};

// WARNING! This implementation is incomplete. For instance escaped unicode
// characters are not supported (e.g. “\uD83D\uDE10”).
// You can find more details here: https://www.ietf.org/rfc/rfc4627.txt
Parser<JsonString> json_string()
{
	Parser<char> escaped_quote = '"' <= string_("\\\"");
	Parser<char> non_quote_char = satisfy([](char x) { return x != '"'; });
	return prefix_parsing_failure(
		"JsonString",
		(function(make_json_string) < function(chars_to_string<vector>))
		^ char_('"') >> some(escaped_quote || non_quote_char) << char_('"')
	);
}

Parser<string> spacer()
{
	Parser<char> spacer_char = satisfy([](char c) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	});
	return function(chars_to_string<vector>) ^ some(spacer_char);
}

Parser<JsonArray> json_array()
{
	/*return prefix_parsing_failure(
		"JsonArray"
	);*/
}

Parser<JsonObject> json_object()
{
	/*return prefix_parsing_failure(
		"JsonObject"
	);*/
}

template <typename T>
inline JsonValue make_json_value(T x)
{
	return JsonValue{x};
}

Parser<JsonValue> json_value()
{
	return prefix_parsing_failure(
		"JsonValue",
		many(spacer()) >> (
			(function(make_json_value<JsonNull>) ^ json_null())
			|| (function(make_json_value<JsonBool>) ^ json_bool())
			|| (function(make_json_value<JsonNumber>) ^ json_number())
			|| (function(make_json_value<JsonString>) ^ json_string())
			|| (function(make_json_value<JsonArray>) ^ json_array())
			|| (function(make_json_value<JsonObject>) ^ json_object())
		) << many(spacer())
	);
}

// }}}1
