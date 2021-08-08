#ifndef _JSON_DATA_MODELING_TYPES_HPP_
#define _JSON_DATA_MODELING_TYPES_HPP_

#include <functional>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "json/types.hpp"
#include "parser/types.hpp"

using namespace std;


// “FromJSON” {{{1

using JsonPath = list<string>;

// “JsonValue” as an input for parsing and current JSON path for error debug info
using FromJsonInput = pair<JsonValue, JsonPath>;

// Parser type for parsing some concrete type out of “JsonValue”
template <typename A>
struct FromJsonParser: function<ParsingResult<A, FromJsonInput>(FromJsonInput)> {};

template <>
struct ParserInput<FromJsonParser> { FromJsonInput input_type; };

// Helpers {{{2

inline FromJsonInput make_from_json_input(JsonValue x)
{
	JsonPath json_path;
	return make_pair(x, json_path);
}

// }}}2

// }}}1


// Type class instances-ish for “FromJsonParser” type {{{1

// MonadFail
template <typename A = Unit>
inline FromJsonParser<A> fail(string err)
{
	return fail<A, FromJsonParser>(err);
}

// Functor
template <typename A, typename B>
inline FromJsonParser<B> fmap(function<B(A)> map_fn, FromJsonParser<A> parser)
{
	return fmap<A, B, FromJsonParser>(map_fn, parser);
}

// Applicative
template <typename A>
inline FromJsonParser<A> pure(A x)
{
	return pure<A, FromJsonParser>(x);
}

// Applicative
template <typename A, typename B>
inline FromJsonParser<B> apply(
	FromJsonParser<function<B(A)>> fn_parser,
	FromJsonParser<A> parser
)
{
	return apply<A, B, FromJsonParser>(fn_parser, parser);
}

// Alternative
template <typename A>
inline FromJsonParser<A> alt(
	FromJsonParser<A> parser_a,
	FromJsonParser<A> parser_b
)
{
	return alt<A, FromJsonParser>(parser_a, parser_b);
}
// Alternative
template <typename A>
inline FromJsonParser<vector<A>> some(FromJsonParser<A> parser)
{
	return some<A, FromJsonParser>(parser);
}

// Alternative
template <typename A>
inline FromJsonParser<vector<A>> many(FromJsonParser<A> parser)
{
	return many<A, FromJsonParser>(parser);
}

template <typename A>
inline FromJsonParser<A> prefix_parsing_failure(string pfx, FromJsonParser<A> p)
{
	return prefix_parsing_failure<A, FromJsonParser>(pfx, p);
}

template <typename A>
inline FromJsonParser<vector<A>> optional_list(FromJsonParser<vector<A>> parser)
{
	return optional_list<A, FromJsonParser>(parser);
}

// }}}1

#endif
