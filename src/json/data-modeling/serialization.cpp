#include <map>
#include <variant>
#include <vector>

#include "../../helpers.hpp"
#include "json/data-modeling/serialization.hpp"
#include "json/types.hpp"

using namespace std;


template <>
JsonValue to_json(JsonObject x)
{
	return make_json_value(x);
}

template <>
JsonValue to_json(JsonArray x)
{
	return make_json_value(x);
}

template <>
JsonValue to_json(JsonString x)
{
	return make_json_value(x);
}

template <>
JsonValue to_json(JsonNumber x)
{
	return make_json_value(x);
}

template <>
JsonValue to_json(JsonBool x)
{
	return make_json_value(x);
}

template <>
JsonValue to_json(JsonNull x)
{
	return make_json_value(x);
}


template <>
JsonValue to_json(Unit x)
{
	return to_json<JsonNull>(JsonNull{x});
}

template <>
JsonValue to_json(bool x)
{
	return to_json<JsonBool>(make_json_bool(x));
}

template <>
JsonValue to_json(variant<int, double> x)
{
	return to_json<JsonNumber>(make_json_number(x));
}

template <>
JsonValue to_json(int x)
{
	return to_json<variant<int, double>>(x);
}

template <>
JsonValue to_json(double x)
{
	return to_json<variant<int, double>>(x);
}

template <>
JsonValue to_json(uint8_t x)
{
	return to_json<int>(x);
}

template <>
JsonValue to_json(string x)
{
	return to_json<JsonString>(make_json_string(x));
}

template <>
JsonValue to_json(vector<JsonValue> x)
{
	return to_json<JsonArray>(make_json_array(x));
}

template <>
JsonValue to_json(map<string, JsonValue> x)
{
	return to_json<JsonObject>(make_json_object(x));
}
