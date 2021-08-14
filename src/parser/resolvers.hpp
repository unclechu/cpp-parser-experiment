#pragma once

// Functions that are useful for initiating parsing and resolving the parsing
// result.

#include <functional>
#include <variant>

#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


// parsing_resolver {{{1

template <typename A, typename B, template<typename>typename F>
// either :: (a → c) → (b → c) → Either a b → c
//   ↑ kind of, the last “c” is more like “ParsingResult c”
// You can also interpret this as “bimap” except that in the second argument you
// don’t need to handle the input tail.
function<B(ParsingResult<A, ParserInputType<F>>)> parsing_resolver(
	function<B(ParsingError<ParserInputType<F>>)> failure_resolve,
	function<B(A)> success_resolve
)
{
	return [=](ParsingResult<A, ParserInputType<F>> result) {
		return visit(overloaded {
			[success_resolve](ParsingSuccess<A, ParserInputType<F>> x) -> B {
				auto [ value, _ ] = x;
				return success_resolve(value);
			},
			[failure_resolve](ParsingError<ParserInputType<F>> err) -> B {
				return failure_resolve(err);
			}
		}, result);
	};
}

// }}}1

// parse {{{1

template <typename A, typename B, template<typename>typename F>
B parse(
	function<B(ParsingResult<A, ParserInputType<F>>)> resolver,
	F<A> parser,
	ParserInputType<F> input
)
{
	return resolver(parser(input));
}

template <typename A, template<typename>typename F>
variant<ParsingError<ParserInputType<F>>, A> parse(
	F<A> parser,
	ParserInputType<F> input
)
{
	using Result = variant<ParsingError<ParserInputType<F>>, A>;
	return parse<A, Result>(
		parsing_resolver<A, Result, F>(
			[](ParsingError<ParserInputType<F>> err) { return err; },
			[](A x) { return x; }
		),
		parser,
		input
	);
}

// }}}1
