#include <utility>
#include <variant>

using namespace std;


#include "json/data-modeling/helpers.hpp"
#include "json/data-modeling/parsers.hpp"
#include "json/data-modeling/types.hpp"

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
