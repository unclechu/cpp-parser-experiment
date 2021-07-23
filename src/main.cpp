#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

#include "test.hpp"

using namespace std;


void show_usage(ostream& out, char* app)
{
	out
		<< "Usage: " << app << " <option(s)> [COMMAND]" << endl
		<< endl
		<< "With no arguments it just parses JSON from stdin." << endl
		<< endl
		<< "Options:" << endl
		<< "  -h, --help  Show this help message" << endl
		<< "  --pretty    Enable pretty printing for output JSON" << endl
		<< endl
		<< "Commands:" << endl
		<< "  test        Run the unit tests" << endl;
}

int parse_json(bool pretty_printer, string json_input)
{
	cerr << "TODO: Implement JSON parsing" << endl;
	cerr << "Debug JSON input: “" << json_input << "”" << endl;
	return EXIT_FAILURE;
}

int main(int argc, char* argv[])
{
	if (argc == 2 && (
		strcmp(argv[1], "--help") == 0 ||
		strcmp(argv[1], "-h") == 0
	)) {
		show_usage(cout, argv[0]);
		return EXIT_SUCCESS;
	} else if (argc == 2 && strcmp(argv[1], "test") == 0) {
		return run_test_cases();
	} else if (argc == 1 || (argc == 2 && strcmp(argv[1], "--pretty") == 0)) {
		ostringstream out;
		string line;
		while (getline(cin, line))
			out << line << endl;
		return parse_json(argc == 2, out.str());
	} else {
		cerr << "Incorrect arguments:";
		for (decltype(argc) i = 1; i < argc; ++i)
			cerr << " ‘" << argv[i] << "’";
		cerr << endl << endl;
		show_usage(cerr, argv[0]);
		return EXIT_FAILURE;
	}
}
