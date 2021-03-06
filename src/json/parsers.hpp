#pragma once

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
variant<ParsingError<ParserInputType<Parser>>, JsonValue> parse_json(
	ParserInputType<Parser> input
);
