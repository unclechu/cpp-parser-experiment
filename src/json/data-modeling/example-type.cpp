#include <map>
#include <utility>
#include <variant>
#include <vector>

using namespace std;


#include "json/data-modeling/helpers.hpp"
#include "json/data-modeling/parsers.hpp"
#include "json/data-modeling/serialization.hpp"
#include "json/data-modeling/types.hpp"
#include "json/types.hpp"

#include "json/data-modeling/example-type.hpp"

#include "parser/resolvers.hpp"


// FromJSON instances-ish {{{1

template <>
FromJsonParser<ExampleTypeAddress> from_json()
{
	return prefix_parsing_failure(
		"ExampleTypeAddress",
		curry_constructor<ExampleTypeAddress, string, string, string, string>()
			^ in_key("streetAddress", from_json<string>())
			^ in_key("city", from_json<string>())
			^ in_key("state", from_json<string>())
			^ in_key("postalCode", from_json<string>())
	);
}

template <>
FromJsonParser<ExampleTypePhoneNumber> from_json()
{
	return prefix_parsing_failure(
		"ExampleTypePhoneNumber",
		curry_constructor<ExampleTypePhoneNumber, string, string>()
			^ in_key("type", from_json<string>())
			^ in_key("number", from_json<string>())
	);
}

template <>
FromJsonParser<ExampleType> from_json()
{
	return prefix_parsing_failure(
		"ExampleType",
		curry_constructor<
			ExampleType,
			string,
			string,
			bool,
			uint8_t,
			ExampleTypeAddress,
			vector<ExampleTypePhoneNumber>
		>()
			^ in_key("firstName", from_json<string>())
			^ in_key("lastName", from_json<string>())
			^ in_key("isAlive", from_json<bool>())
			^ in_key("age", from_json<uint8_t>())
			^ in_key("address", from_json<ExampleTypeAddress>())
			^ in_key("phoneNumbers", from_json<vector, ExampleTypePhoneNumber>(
				from_json<ExampleTypePhoneNumber>()
			))
	);
}

// }}}1

// ToJSON instances-ish {{{1

template <>
JsonValue to_json(ExampleTypeAddress x)
{
	map<string, JsonValue> obj = {
		{"streetAddress", to_json(x.street_address)},
		{"city", to_json(x.city)},
		{"state", to_json(x.state)},
		{"postalCode", to_json(x.postal_code)},
	};
	return to_json(obj);
}

template <>
JsonValue to_json(ExampleTypePhoneNumber x)
{
	map<string, JsonValue> obj = {
		{"type", to_json(x.type)},
		{"number", to_json(x.number)},
	};
	return to_json(obj);
}

template <>
JsonValue to_json(ExampleType x)
{
	map<string, JsonValue> obj = {
		{"firstName", to_json(x.first_name)},
		{"lastName", to_json(x.last_name)},
		{"isAlive", to_json(x.is_alive)},
		{"age", to_json(x.age)},
		{"address", to_json(x.address)},
		{
			"phoneNumbers",
			to_json<vector, ExampleTypePhoneNumber>(x.phone_numbers)
		},
	};
	return to_json(obj);
}

// }}}1
