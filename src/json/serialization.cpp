#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "helpers.hpp"
#include "json/serialization.hpp"
#include "json/types.hpp"

using namespace std;


string serialize_json(JsonString);
string serialize_json(JsonValue, string, string, string);

string serialize_json(
	JsonObject x,
	string line_break,
	string block_indent,
	string cur_indent
)
{
	map<string, JsonValue> map = from_json_object(x);
	if (map.empty()) return "{}";

	ostringstream out;
	out << "{" << line_break;
	bool first (true);

	for (pair<string, JsonValue> x : map) {
		if (first) first = false; else out << "," << line_break;
		out
			<< block_indent << cur_indent
			<< serialize_json(make_json_string(x.first))
			<< ":"
			<< (block_indent.empty() && line_break.empty() ? "" : " ")
			<< serialize_json(
				x.second,
				line_break,
				block_indent,
				cur_indent + block_indent
			);
	}

	out << line_break << cur_indent << "}";
	return out.str();
}

string serialize_json(
	JsonArray x,
	string line_break,
	string block_indent,
	string cur_indent
)
{
	vector<JsonValue> array = from_json_array(x);
	if (array.empty()) return "[]";

	ostringstream out;
	out << "[" << line_break;
	bool first (true);

	for (JsonValue x : array) {
		if (first) first = false; else out << "," << line_break;
		out
			<< block_indent << cur_indent
			<< serialize_json(
				x,
				line_break,
				block_indent,
				cur_indent + block_indent
			);
	}

	out << line_break << cur_indent << "]";
	return out.str();
}

string serialize_json(JsonString x)
{
	ostringstream out;
	out << "\"";
	for (auto &ch : from_json_string(x))
		if (ch == '"')
			out << "\\\"";
		else
			out << ch;
	out << "\"";
	return out.str();
}

string serialize_json(JsonNumber x)
{
	return visit(overloaded {
		[](int x) -> string { return to_string(x); },
		[](double x) -> string { return to_string(x); }
	}, from_json_number(x));
}

string serialize_json(JsonBool x)
{
	return from_json_bool(x) ? "true" : "false";
}

string serialize_json(
	JsonValue json,
	string line_break,
	string block_indent,
	string cur_indent
)
{
	return visit(overloaded {
		[line_break, block_indent, cur_indent](JsonObject x) -> string {
			return serialize_json(x, line_break, block_indent, cur_indent);
		},
		[line_break, block_indent, cur_indent](JsonArray x) -> string {
			return serialize_json(x, line_break, block_indent, cur_indent);
		},
		[](JsonString x) -> string { return serialize_json(x); },
		[](JsonNumber x) -> string { return serialize_json(x); },
		[](JsonBool x) -> string { return serialize_json(x); },
		[](JsonNull) -> string { return "null"; }
	}, from_json_value(json));
}

string serialize_json(JsonValue json, string line_break, string block_indent)
{
	return serialize_json(json, line_break, block_indent, "");
}

string serialize_json(JsonValue json)
{
	return serialize_json(json, "", "");
}
