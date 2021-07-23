#ifndef _PARSER_ALTERNATIVE_HPP_
#define _PARSER_ALTERNATIVE_HPP_

// Alternative implementation (mimicking Alternative type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   <|>      → ||
//   some     → some
//   many     → many
//   optional → optional_parser (“optional” is already taken by STL)

#include <functional>
#include <optional>
#include <vector>

#include "abstractions/applicative.hpp"
#include "abstractions/functor.hpp"
#include "helpers.hpp"

using namespace std;


// alt {{{1

template <template<typename>typename F, typename A>
F<A> alt(F<A> functor_a, F<A> functor_b)
{
	return alt<A>(functor_a, functor_b);
}

template <template<typename>typename F, typename A>
// Operator equivalent for “alt”
F<A> operator||(F<A> a, F<A> b)
{
	return alt<F, A>(a, b);
}

// }}}1

// some {{{1

template <template<typename>typename F, typename A>
// One or more.
// some :: Alternative f => f a -> f [a]
// WARNING! Do not apply on parsers that are always successful (like “pure(…)”)
// The recursion will never end in this case. It doesn’t mean you can’t compose
// “pure” with parsers that are used with this function. The only point is that
// parser must fail at some point to finalize the resulting list.
F<vector<A>> some(F<A> functor)
{
	return some<A>(functor);
}

// }}}1

// many {{{1

template <template<typename>typename F, typename A>
// Zero or more.
// many :: Alternative f => f a -> f [a]
// WARNING! Do not apply on parsers that are always successful (like “pure(…)”)
// The recursion will never end in this case. It doesn’t mean you can’t compose
// “pure” with parsers that are used with this function. The only point is that
// parser must fail at some point to finalize the resulting list.
F<vector<A>> many(F<A> functor)
{
	return many<A>(functor);
}

// }}}1

// optional_parser {{{1

template<template<typename>typename F, typename A>
// optional :: Alternative f => f a -> f (Maybe a)
F<optional<A>> optional_parser(F<A> functor)
{
	return (function(to_optional<A>) ^ functor) || pure<F, optional<A>>(nullopt);
}

// }}}1

#endif
