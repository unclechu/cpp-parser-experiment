#pragma once

#include <string>

#include "json/types.hpp"

using namespace std;


string serialize_json(JsonValue, string line_separator, string block_indent);
string serialize_json(JsonValue);
