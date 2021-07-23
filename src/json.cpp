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


Parser<JsonNull> json_null()
{
	return prefix_parsing_failure(
		"json_null",
		parse_string("null") >= JsonNull{unit()}
	);
}

Parser<JsonBool> json_bool()
{
	function<JsonBool(bool)> fn = [](bool x) {
		return JsonBool{make_tuple(x)};
	};
	return prefix_parsing_failure(
		"json_bool",
		fn ^ (parse_string("true") >= true || parse_string("false") >= false)
	);
}
