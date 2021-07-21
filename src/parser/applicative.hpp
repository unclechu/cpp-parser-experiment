#ifndef _PARSER_APPLICATIVE_HPP_
#define _PARSER_APPLICATIVE_HPP_

// Applicative implementation (mimicking Applicative type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   <*> → ^
//   <*  → <<
//   *>  → >>

#include <functional>
#include <tuple>
#include <variant>

#include "helpers.hpp"
#include "parser/functor.hpp"
#include "parser/types.hpp"

using namespace std;


// pure {{{1

template <typename A>
// pure :: Applicative f => a -> Parser a
Parser<A> pure(A x)
{
	return Parser<A>{[=](Input input) { return make_tuple(x, input); }};
}

// }}}1


// apply {{{1

template <typename A, typename B>
// (<*>) :: Parser (a → b) → Parser a → Parser b
Parser<B> apply(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return Parser<B>{[=](Input input) -> ParsingResult<B> {
		return visit(overloaded {
			[=](ParsingSuccess<function<B(A)>> x) -> ParsingResult<B> {
				auto [ fn, tail ] = x;
				return fmap_parser<A, B>(fn, parser)(tail);
			},
			[](ParsingError err) -> ParsingResult<B> { return err; }
		}, fn_parser(input));
	}};
}

template <typename A, typename B>
// Operator equivalent for “apply”
Parser<B> operator^(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return apply<A, B>(fn_parser, parser);
}

// }}}1


// apply arrows {{{1

// apply_first {{{2

template <typename A, typename B>
// (<*) :: Parser a → Parser b → Parser a
Parser<A> apply_first(Parser<A> parser_a, Parser<B> parser_b)
{
	// (\a _ -> a) <$> parser_a <*> parser_b
	return apply<B, A>(
		fmap_parser<A, function<A(B)>>(
			[](A a) { return [=](B) { return a; }; },
			parser_a
		),
		parser_b
	);
}

template <typename A, typename B>
// Operator equivalent for “apply_first”
Parser<A> operator<<(Parser<A> parser_a, Parser<B> parser_b)
{
	return apply_first<A, B>(parser_a, parser_b);
}

// }}}2

// apply_second {{{2

template <typename A, typename B>
// (*>) :: Parser a → Parser b → Parser b
Parser<B> apply_second(Parser<A> parser_a, Parser<B> parser_b)
{
	// (\_ b -> b) <$> parser_a <*> parser_b
	return apply<B, B>(
		fmap_parser<A, function<B(B)>>(
			[](A) { return [](B b) { return b; }; },
			parser_a
		),
		parser_b
	);
}

template <typename A, typename B>
// Operator equivalent for “apply_second”
Parser<B> operator>>(Parser<A> parser_a, Parser<B> parser_b)
{
	return apply_second<A, B>(parser_a, parser_b);
}

// }}}2

// }}}1

#endif
