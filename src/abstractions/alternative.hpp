#ifndef _ABSTRACTIONS_ALTERNATIVE_HPP_
#define _ABSTRACTIONS_ALTERNATIVE_HPP_

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

// additionals {{{1

template <template<typename>typename F, typename A>
// Alternative version of “some”.
// It takes one applicative for the first element (head) of the resulting list
// and another one for all the other elements (tail) of the list.
F<vector<A>> one_plus(F<A> head, F<A> tail)
{
	function<vector<A>(A, vector<A>)> cons = [](A x, vector<A> xs) {
		vector list {x};
		list.insert(list.end(), xs.begin(), xs.end());
		return list;
	};
	return curry(cons) ^ head ^ many(tail);
}

template <template<typename>typename F, typename A, typename S>
// Alternative version of “some”.
// Second applicative is a separator between all elements.
F<vector<A>> separated_some(F<A> a, F<S> separator)
{
	return one_plus<F, A>(a, separator >> a);
}

template<template<typename>typename F, typename A>
// optional :: Alternative f => f a -> f (Maybe a)
F<optional<A>> optional_parser(F<A> functor)
{
	return (function(to_optional<A>) ^ functor) || pure<F, optional<A>>(nullopt);
}

// }}}1

#endif
