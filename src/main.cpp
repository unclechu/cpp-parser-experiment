#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <variant>

#include "helpers.hpp"
#include "json/parsers.hpp"
#include "json/serialization.hpp"
#include "json/types.hpp"
#include "parser/resolvers.hpp"
#include "parser/types.hpp"
#include "test.hpp"

#include "json/data-modeling/example-type.hpp"
#include "json/data-modeling/parsers.hpp"
#include "json/data-modeling/serialization.hpp"

using namespace std;


void show_usage(ostream& out, char* app)
{
	out
		<< endl
		<< "Usage: " << app << " <option(s)> [COMMAND]" << endl
		<< endl
		<< "With no arguments it just parses JSON from stdin." << endl
		<< endl
		<< "Options:" << endl
		<< "  -h, --help  Show this help message" << endl
		<< "  --pretty    Enable pretty printing for output JSON" << endl
		<< endl
		<< "  --model     Apply parsing from JSON into a data model" << endl
		<< "              and then apply serialization back to JSON" << endl
		<< "              (mind that it works only with data from" << endl
		<< "              example.json file and ignores a couple of" << endl
		<< "              fields there that were just “null” and an" << endl
		<< "              empty list of unknown type)" << endl
		<< endl
		<< "Commands:" << endl
		<< "  test        Run the unit tests" << endl
		<< endl;
}

string serialize_json_to_string(bool pretty_print, JsonValue x)
{
	if (pretty_print) {
		ostringstream line_break;
		line_break << endl;
		return serialize_json(x, line_break.str(), "  ");
	} else {
		return serialize_json(x);
	}
}

JsonValue parse_json_and_resolve_result(string json_input)
{
	return visit(overloaded {
		[](ParsingError<ParserInputType<Parser>> err) -> JsonValue {
			cerr
				<< "Failed to parse JSON: " << err.first << endl
				<< "Input tail: " << quoted(err.second) << endl;
			exit(EXIT_FAILURE);
		},
		[](JsonValue x) -> JsonValue { return x; }
	}, parse_json(json_input));
}

ExampleType parse_example_type_and_resolve_result(JsonValue json_input)
{
	variant<
		ParsingError<ParserInputType<FromJsonParser>>,
		ExampleType
	> x =
		parse<ExampleType, FromJsonParser>(
			from_json<ExampleType>(),
			make_from_json_input(json_input)
		);

	return visit(overloaded {
		[](ParsingError<ParserInputType<FromJsonParser>> err) -> ExampleType {
			string json_path;
			{
				ostringstream out;
				bool first = true;
				for (auto x : err.second.second) {
					if (first) {
						out << x;
						first = false;
					} else {
						out << "." << x;
					}
				}
				json_path = out.str();
			}

			cerr
				// TODO substitute type name
				<< "Failed to parse ExampleType: " << err.first << endl
				<< "JSON path: "
				<< quoted(json_path.empty() ? "(root)" : json_path) << endl
				<< "Input JSON: "
				<< serialize_json_to_string(true, err.second.first) << endl;

			exit(EXIT_FAILURE);
		},
		[](ExampleType y) -> ExampleType { return y; }
	}, x);
}

string slurp_stdin()
{
	ostringstream out;
	string line;
	while (getline(cin, line)) out << line << endl;
	return out.str();
}

void show_incorrect_arguments_error(int argc, char* argv[])
{
	cerr << "Incorrect arguments:";
	for (decltype(argc) i = 1; i < argc; ++i) cerr << " " << quoted(argv[i]);
	cerr << endl;
	show_usage(cerr, argv[0]);
}

int main(int argc, char* argv[])
{
	bool show_help = false;
	bool pretty_print = false;
	bool modeled_data = false;
	bool run_tests = false;

	for (decltype(argc) i = 1; i < argc; ++i) {
		// It’s always okay to call “--help” at any point
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			show_help = true;
		}
		// “test” sub-command
		else if (strcmp(argv[i], "test") == 0) {
			if (run_tests && (pretty_print || modeled_data)) {
				show_incorrect_arguments_error(argc, argv);
				return EXIT_FAILURE;
			} else {
				run_tests = true;
			}
		}
		// If there were “test” sub-command any other argument is incorrect
		else if (run_tests) {
			show_incorrect_arguments_error(argc, argv);
			return EXIT_FAILURE;
		}
		// Print “pretty” human-readable JSON instead of one-line JSON
		else if (strcmp(argv[i], "--pretty") == 0) {
			pretty_print = true;
		}
		// Also parse “ExampleType” from parsed JSON
		else if (strcmp(argv[i], "--model") == 0) {
			modeled_data = true;
		}
		else {
			show_incorrect_arguments_error(argc, argv);
			return EXIT_FAILURE;
		}
	}

	if (show_help) {
		show_usage(cout, argv[0]);
		return EXIT_SUCCESS;
	}
	else if (run_tests) {
		return run_test_cases();
	}
	else {
		JsonValue json = parse_json_and_resolve_result(slurp_stdin());

		if (modeled_data) {
			ExampleType x = parse_example_type_and_resolve_result(json);
			cout << serialize_json_to_string(pretty_print, to_json(x)) << endl;
		} else {
			cout << serialize_json_to_string(pretty_print, json) << endl;
		}

		return EXIT_SUCCESS;
	}
}
