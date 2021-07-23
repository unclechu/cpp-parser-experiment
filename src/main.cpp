#include <iostream>
#include <string.h>

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
	} else if (argc == 1) {
		cerr << "TODO: Implement JSON parsing" << endl;
		return EXIT_FAILURE;
	} else if (argc == 2 && strcmp(argv[1], "--pretty") == 0) {
		cerr << "TODO: Implement JSON parsing with pretty printing" << endl;
		return EXIT_FAILURE;
	} else {
		cerr << "Incorrect arguments:";
		for (int i = 1; i < argc; ++i) cerr << " ‘" << argv[i] << "’";
		cerr << endl << endl;
		show_usage(cerr, argv[0]);
		return EXIT_FAILURE;
	}
}
