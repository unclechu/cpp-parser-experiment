#ifndef _JSON_HPP_
#define _JSON_HPP_

#include <map>
#include <memory>
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


// Parsers
Parser<JsonNull> json_null();
Parser<JsonBool> json_bool();
Parser<JsonNumber> json_number();
Parser<JsonString> json_string();

#endif
