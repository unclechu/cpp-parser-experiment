#ifndef _PARSER_HELPERS_HPP_
#define _PARSER_HELPERS_HPP_

// Some helpers for parsers

#include <functional>
#include <variant>

#include "../helpers.hpp"
#include "parser/types.hpp"

using namespace std;


template <typename A>
Parser<A> map_parsing_failure(
	function<ParsingError(ParsingError)> map_fn,
	Parser<A> parser
)
{
	return Parser<A>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingError err) -> ParsingResult<A> { return map_fn(err); },
			[](ParsingSuccess<A> x) -> ParsingResult<A> { return x; }
		}, parser(input));
	}};
}

template <typename A>
Parser<A> prefix_parsing_failure(string pfx, Parser<A> parser)
{
	return map_parsing_failure<A>(
		[pfx](ParsingError err) { return ParsingError{pfx + ": " + err}; },
		parser
	);
}

#endif
