#ifndef _PARSER_HELPERS_HPP_
#define _PARSER_HELPERS_HPP_

// Some helpers for parsers

#include <functional>
#include <optional>
#include <variant>

#include "../helpers.hpp"
#include "parser/types.hpp"

template<typename A>
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
function<A(A)> const_map(A x)
{
	return [=](A) { return x; };
}

template <typename A>
optional<A> to_optional(A x)
{
	return x;
};

#endif
