#ifndef _JSON_TYPES_HPP_
#define _JSON_TYPES_HPP_

#include <map>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


struct JsonValue; // Algebraic data type

// Constructors-ish
struct JsonObject: tuple<map<string, JsonValue>> {};
struct JsonArray: tuple<vector<JsonValue>> {};
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


// Wrappers and unwrappers {{{1

// JsonObject
inline JsonObject make_json_object(map<string, JsonValue> x)
{
	return JsonObject{x};
};
inline map<string, JsonValue> from_json_object(JsonObject x)
{
	return get<0>(x);
}

// JsonArray
inline JsonArray make_json_array(vector<JsonValue> x)
{
	return JsonArray{x};
};
inline vector<JsonValue> from_json_array(JsonArray x)
{
	return get<0>(x);
}

// JsonString
inline JsonString make_json_string(string x)
{
	return JsonString{x};
};
inline string from_json_string(JsonString x)
{
	return get<0>(x);
}

// JsonNumber
template <typename T>
inline JsonNumber make_json_number(T x)
{
	return JsonNumber{x};
}
inline variant<int, double> from_json_number(JsonNumber x)
{
	return get<0>(x);
}

// JsonBool
inline JsonBool make_json_bool(bool x)
{
	return JsonBool{x};
}
inline bool from_json_bool(JsonBool x)
{
	return get<0>(x);
}

// JsonValue
template <typename T>
inline JsonValue make_json_value(T x)
{
	return JsonValue{x};
}
inline variant<
	JsonObject,
	JsonArray,
	JsonString,
	JsonNumber,
	JsonBool,
	JsonNull
> from_json_value(JsonValue x)
{
	return x;
}

// }}}1

// Helpers {{{1

template <typename K, typename V>
inline map<K, V> make_map_from_vector(vector<tuple<K, V>> list)
{
	map<K, V> result;
	for (auto [ k, v ] : list) result.insert(make_pair(k, v));
	return result;
}

// }}}1

#endif
