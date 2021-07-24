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
#include "parser/resolvers.hpp"
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

string from_json_string(JsonString x)
{
	return get<0>(x);
}

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
	return function(chars_to_string<vector>) ^ many(spacer_char);
}

inline JsonArray make_json_array(vector<JsonValue> x)
{
	return JsonArray{make_tuple(x)};
}

template <typename T>
// Resolved to empty list by default
Parser<vector<T>> optional_list(Parser<vector<T>> parser)
{
	vector<T> empty_list;
	return parser || pure<decltype(empty_list)>(empty_list);
}

// Lazy evaluation (avoid infinite recursion)
inline Parser<JsonValue> lazy_json_value()
{
	return Parser<JsonValue>{[](Input input) { return json_value()(input); }};
}

Parser<JsonArray> json_array()
{
	Parser<char> separator = spacer() >> char_(',') << spacer();
	Parser<vector<JsonValue>> elements =
		separated_some(lazy_json_value(), separator);
	return prefix_parsing_failure(
		"JsonArray",
		char_('[') >> spacer()
		>> (function(make_json_array) ^ optional_list(elements))
		<< spacer() << char_(']')
	);
}

inline JsonObject make_json_object(map<string, JsonValue> x)
{
	return JsonObject{x};
}

template <typename K, typename V>
inline map<K, V> make_map_from_vector(vector<tuple<K, V>> list)
{
	map<K, V> result;
	for (auto [ k, v ] : list) result.insert(make_pair(k, v));
	return result;
}

Parser<JsonObject> json_object()
{
	Parser<char> separator = spacer() >> char_(',') << spacer();
	using Entry = tuple<string, JsonValue>;

	Parser<Entry> entry =
		function(curry<string, JsonValue, Entry>(make_tuple<string, JsonValue>))
		^ (function(from_json_string) ^ json_string()) << spacer() << char_(':')
		^ spacer() >> lazy_json_value();

	Parser<map<string, JsonValue>> entries =
		function(make_map_from_vector<string, JsonValue>)
		^ separated_some(entry, separator);

	return prefix_parsing_failure(
		"JsonObject",
		function(make_json_object)
			^ char_('{') >> spacer() >> entries << spacer() << char_('}')
	);
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
		spacer() >> (
			(function(make_json_value<JsonNull>) ^ json_null())
			|| (function(make_json_value<JsonBool>) ^ json_bool())
			|| (function(make_json_value<JsonNumber>) ^ json_number())
			|| (function(make_json_value<JsonString>) ^ json_string())
			|| (function(make_json_value<JsonArray>) ^ json_array())
			|| (function(make_json_value<JsonObject>) ^ json_object())
		) << spacer()
	);
}

// }}}1


// Parsing {{{1

variant<ParsingError, JsonValue> parse_json(Input input)
{
	return parse<JsonValue>(json_value() << end_of_input(), input);
}

// }}}1
