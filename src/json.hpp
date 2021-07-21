#ifndef _JSON_HPP_
#define _JSON_HPP_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <variant>

#include "helpers.hpp"

using namespace std;


struct JsonValue; // Algebraic data type

// Constructors-ish
struct JsonObject: tuple<map<string, JsonValue>> {};
struct JsonArray: tuple<list<JsonValue>> {};
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

#endif
