#ifndef _PARSER_FUNCTOR_HPP_
#define _PARSER_FUNCTOR_HPP_

// Functor implementation (mimicking Functor type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   fmap → fmap (“map” and “fmap” are occupied by STL)
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
// (<$>) :: (a → b) → f a → f b
// (<$>) :: (a → b) → Parser a → Parser b
Parser<B> fmap(function<B(A)> map_fn, Parser<A> parser)
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

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “fmap”
F<B> operator^(function<B(A)> map_fn, F<A> functor)
{
	return fmap<A, B>(map_fn, functor);
}

template <template<typename>typename F, typename A, typename B>
// Flipped version of “fmap” (like (<&>) comparing to (<$>))
F<B> operator&(F<A> functor, function<B(A)> map_fn)
{
	return fmap<A, B>(map_fn, functor);
}

// }}}1


// voids {{{1

// void_right {{{2

template <template<typename>typename F, typename A, typename B>
// (<$) :: a → f b → f a
F<A> void_right(A to_value, F<B> functor)
{
	return fmap<B, A>([=](B) { return to_value; }, functor);
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “void_right”
F<A> operator<=(A to_value, F<B> functor)
{
	return void_right<F, A, B>(to_value, functor);
}

// }}}2

// void_left {{{2

template <template<typename>typename F, typename A, typename B>
// ($>) :: f a → b → f b
F<B> void_left(F<A> functor, B to_value)
{
	return void_right(to_value, functor);
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “void_left”
F<B> operator>=(F<A> functor, B to_value)
{
	return void_left<F, A, B>(functor, to_value);
}

// }}}2

// }}}1

#endif
