#ifndef _PARSER_FUNCTOR_HPP_
#define _PARSER_FUNCTOR_HPP_

// Functor implementation (mimicking Functor type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   fmap → fmap_parser (“map” and “fmap” are occupied by STL)
//   <$>  → ^
//   <&>  → &
//   <$   → <=
//   $>   → >=

#include <functional>
#include <tuple>
#include <variant>

#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


// fmap {{{1

template <typename A, typename B>
// (<$>) :: (a → b) → Parser a → Parser b
Parser<B> fmap_parser(function<B(A)> map_fn, Parser<A> parser)
{
	return Parser<B>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingSuccess<A> x) -> ParsingResult<B> {
				auto [ value, tail ] = x;
				return make_tuple(map_fn(value), tail);
			},
			[](ParsingError err) -> ParsingResult<B> { return err; },
		}, parser(input));
	}};
}

template <typename A, typename B>
// Operator equivalent for “fmap_parser”
Parser<B> operator^(function<B(A)> map_fn, Parser<A> parser)
{
	return fmap_parser<A, B>(map_fn, parser);
}

template <typename A, typename B>
// Flipped version of “fmap” (like (<&>) comparing to (<$>))
Parser<B> operator&(Parser<A> parser, function<B(A)> map_fn)
{
	return fmap_parser<A, B>(map_fn, parser);
}

// }}}1


// voids {{{1

// void_right {{{2

template <typename A, typename B>
// (<$) :: a → Parser b → Parser a
Parser<A> void_right(A to_value, Parser<B> parser)
{
	return fmap_parser<B, A>([=](B) { return to_value; }, parser);
}

template <typename A, typename B>
// Operator equivalent for “void_right”
Parser<A> operator<=(A to_value, Parser<B> parser)
{
	return void_right<A, B>(to_value, parser);
}

// }}}2

// void_left {{{2

template <typename A, typename B>
// ($>) :: Parser a → b → Parser b
Parser<B> void_left(Parser<A> parser, B to_value)
{
	return void_right(to_value, parser);
}

template <typename A, typename B>
// Operator equivalent for “void_left”
Parser<B> operator>=(Parser<A> parser, B to_value)
{
	return void_left<A, B>(parser, to_value);
}

// }}}2

// }}}1

#endif
