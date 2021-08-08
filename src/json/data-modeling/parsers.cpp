#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "abstractions/functor.hpp"
#include "json/data-modeling/parsers.hpp"

using namespace std;

// Local shorthand
using I = ParserInputType<FromJsonParser>;


// JSON {{{1

template <typename T>
inline FromJsonParser<T> parse_raw_json(string type_name)
{
	return prefix_parsing_failure<T>(
		type_name,
		FromJsonParser<T>{
			[type_name](I input) -> ParsingResult<T, I> {
				return visit([type_name, input](auto&& value) -> ParsingResult<T, I> {
					using ValueT = decay_t<decltype(value)>;
					if constexpr (is_same_v<ValueT, T>)
						return make_parsing_success<T, I>(value, input);
					else
						return make_parsing_error<I>(
							// TODO show actual type name here
							"It’s not a " + type_name + " but some other JSON type",
							input
						);
				}, from_json_value(input.first));
			}
		}
	);
}

template <>
FromJsonParser<JsonObject> from_json()
{
	return parse_raw_json<JsonObject>("JsonObject");
}

template <>
FromJsonParser<JsonArray> from_json()
{
	return parse_raw_json<JsonArray>("JsonArray");
}

template <>
FromJsonParser<JsonString> from_json()
{
	return parse_raw_json<JsonString>("JsonString");
}

template <>
FromJsonParser<JsonNumber> from_json()
{
	return parse_raw_json<JsonNumber>("JsonNumber");
}

template <>
FromJsonParser<JsonBool> from_json()
{
	return parse_raw_json<JsonBool>("JsonBool");
}

template <>
FromJsonParser<JsonNull> from_json()
{
	return parse_raw_json<JsonNull>("JsonNull");
}

// Parsing raw types from JSON types {{{2

template <>
FromJsonParser<map<string, JsonValue>> from_json()
{
	return function(from_json_object) ^ from_json<JsonObject>();
}

template <>
FromJsonParser<vector<JsonValue>> from_json()
{
	return function(from_json_array) ^ from_json<JsonArray>();
}

template <>
FromJsonParser<string> from_json()
{
	return function(from_json_string) ^ from_json<JsonString>();
}

template <>
FromJsonParser<variant<int, double>> from_json()
{
	return function(from_json_number) ^ from_json<JsonNumber>();
}

template <typename T>
inline FromJsonParser<T> parse_number_helper(string type_name) {
	return prefix_parsing_failure<T>(
		type_name,
		FromJsonParser<T>{[type_name](I input) {
			return visit(overloaded {
				[](ParsingError<I> err) -> ParsingResult<T, I> { return err; },
				[type_name, input](
					ParsingSuccess<variant<int, double>, I> x
				) -> ParsingResult<T, I> {
					return visit([type_name, input, x](
						auto&& value
					) -> ParsingResult<T, I> {
						using ValueT = decay_t<decltype(value)>;

						if constexpr (is_same_v<ValueT, T>)
							return make_parsing_success<T, I>(value, x.second);

						// It’s okay to to cast “int” to “double”
						else if constexpr (
							is_same_v<T, int> && is_same_v<ValueT, double>
						)
							return make_parsing_success<T, I>(value, x.second);

						else
							return make_parsing_error<I>(
								"JsonNumber: Failed to extract " + type_name +
								" from variant<int, double>",
								input
							);
					}, x.first);
				}
			}, from_json<variant<int, double>>()(input));
		}}
	);
}

template <>
FromJsonParser<int> from_json()
{
	return parse_number_helper<int>("int");
}

template <>
FromJsonParser<double> from_json()
{
	return parse_number_helper<double>("double");
}

template <>
FromJsonParser<uint8_t> from_json()
{
	return prefix_parsing_failure<uint8_t>(
		"uint8_t",
		FromJsonParser<uint8_t>{[](I input) {
			return visit(overloaded {
				[](ParsingError<I> err) -> ParsingResult<uint8_t, I> {
					return err;
				},
				[input](ParsingSuccess<int, I> x) -> ParsingResult<uint8_t, I> {
					uint8_t y = x.first;
					if (x.first == y)
						return make_parsing_success<uint8_t, I>(y, x.second);
					else
						return make_parsing_error<I>(
							"Failed to get uint8_t from int (the number " +
							to_string(x.first) +
							" either overflows or underflows uint8_t)",
							input
						);
				}
			}, from_json<int>()(input));
		}}
	);
}

template <>
FromJsonParser<bool> from_json()
{
	return function(from_json_bool) ^ from_json<JsonBool>();
}

template <>
FromJsonParser<Unit> from_json()
{
	return unit() <= from_json<JsonNull>();
}

// }}}2

// }}}1
