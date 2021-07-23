#ifndef _RESOLVERS_HPP_
#define _RESOLVERS_HPP_

// Functions that are useful for initiating parsing and resolving the parsing
// result.

#include <functional>
#include <variant>

#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


// parsing_resolver {{{1

template <typename A, typename B>
// either :: (a → c) → (b → c) → Either a b → c
//   ↑ kind of, the last “c” is more like “ParsingResult c”
// You can also interpret this as “bimap” except that in the second argument you
// don’t need to handle the “Input” tail.
function<B(ParsingResult<A>)> parsing_resolver(
	function<B(ParsingError)> failure_resolve,
	function<B(A)> success_resolve
)
{
	return [=](ParsingResult<A> result) {
		return visit(overloaded {
			[=](ParsingSuccess<A> x) -> B {
				auto [ value, _ ] = x;
				return success_resolve(value);
			},
			[=](ParsingError err) -> B {
				return failure_resolve(err);
			}
		}, result);
	};
}

// }}}1

// parse {{{1

template <typename A, typename B>
B parse(function<B(ParsingResult<A>)> resolver, Parser<A> parser, Input input)
{
	return resolver(parser(input));
}

template <typename A>
variant<ParsingError, A> parse(Parser<A> parser, Input input)
{
	using Result = variant<ParsingError, A>;
	return parse<A, Result>(
		parsing_resolver<A, Result>(
			[](ParsingError err) { return err; },
			[](A x) { return x; }
		),
		parser,
		input
	);
}

// }}}1

#endif
