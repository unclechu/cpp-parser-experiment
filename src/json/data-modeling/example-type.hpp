#pragma once

#include <string>
#include <vector>

#include "helpers.hpp"
#include "json/data-modeling/helpers.hpp"
#include "json/data-modeling/parsers.hpp"
#include "json/data-modeling/types.hpp"
#include "json/types.hpp"

#include "abstractions/alternative.hpp"
#include "abstractions/applicative.hpp"
#include "abstractions/functor.hpp"

using namespace std;


// This is a model of data from “example.json” file

struct ExampleTypeAddress {
	string street_address;
	string city;
	string state;
	string postal_code;
};

struct ExampleTypePhoneNumber {
	string type;
	string number;
};

struct ExampleType {
	string first_name;
	string last_name;
	bool is_alive;
	uint8_t age;
	ExampleTypeAddress address;
	vector<ExampleTypePhoneNumber> phone_numbers;
};
