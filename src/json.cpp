#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <variant>

#include "helpers.hpp"
#include "json.hpp"
#include "parser/alternative.hpp"
#include "parser/applicative.hpp"
#include "parser/functor.hpp"
#include "parser/helpers.hpp"
#include "parser/parsers.hpp"
#include "parser/types.hpp"

using namespace std;


// Parsers {{{1

Parser<JsonNull> json_null()
{
	return prefix_parsing_failure(
		"json_null",
		string_("null") >= JsonNull{unit()}
	);
}

Parser<JsonBool> json_bool()
{
	function<JsonBool(bool)> fn = [](bool x) {
		return JsonBool{make_tuple(x)};
	};
	return prefix_parsing_failure(
		"json_bool",
		fn ^ (string_("true") >= true || string_("false") >= false)
	);
}

Parser<string> spacer()
{
	function<bool(char)> is_spacer = [](char c) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	};
	return function(chars_to_string<vector>) ^ some(satisfy(is_spacer));
}

// }}}1
