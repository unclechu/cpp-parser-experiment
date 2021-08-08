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
#include "json/parsers.hpp"
#include "json/types.hpp"
#include "parser/parsers.hpp"
#include "parser/resolvers.hpp"
#include "parser/types.hpp"

using namespace std;


Parser<JsonNull> json_null()
{
	return prefix_parsing_failure(
		"JsonNull",
		string_("null") >= JsonNull{unit()}
	);
}

Parser<JsonBool> json_bool()
{
	return prefix_parsing_failure(
		"JsonBool",
		function(make_json_bool)
		^ (string_("true") >= true || string_("false") >= false)
	);
}

Parser<JsonNumber> json_number()
{
	return prefix_parsing_failure(
		"JsonNumber",
		(function(make_json_number<double>) ^ signed_fractional()) ||
		(function(make_json_number<int>) ^ signed_decimal())
	);
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

// Lazy evaluation (avoid infinite recursion)
inline Parser<JsonValue> lazy_json_value()
{
	return Parser<JsonValue>{[](auto input) { return json_value()(input); }};
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

Parser<JsonObject> json_object()
{
	Parser<char> separator = spacer() >> char_(',') << spacer();
	using Entry = tuple<string, JsonValue>;

	Parser<Entry> entry =
		function(curry<Entry, string, JsonValue>(make_tuple<string, JsonValue>))
		^ (function(from_json_string) ^ json_string()) << spacer() << char_(':')
		^ spacer() >> lazy_json_value();

	Parser<map<string, JsonValue>> entries =
		function(make_map_from_vector<string, JsonValue>)
		^ optional_list(separated_some(entry, separator));

	return prefix_parsing_failure(
		"JsonObject",
		function(make_json_object)
		^ char_('{') >> spacer() >> entries << spacer() << char_('}')
	);
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

variant<ParsingError<ParserInputType<Parser>>, JsonValue> parse_json(
	ParserInputType<Parser> input
)
{
	return parse<JsonValue>(json_value() << end_of_input(), input);
}
