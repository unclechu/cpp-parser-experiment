#ifndef _JSON_PARSERS_HPP_
#define _JSON_PARSERS_HPP_

#include "json/types.hpp"
#include "parser/types.hpp"


// Parsers
Parser<JsonNull> json_null();
Parser<JsonBool> json_bool();
Parser<JsonNumber> json_number();
Parser<JsonString> json_string();
Parser<JsonArray> json_array();
Parser<JsonObject> json_object();
Parser<JsonValue> json_value();

// Parsing
variant<ParsingError, JsonValue> parse_json(Input input);

#endif
