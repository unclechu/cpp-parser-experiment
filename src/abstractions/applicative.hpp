#ifndef _PARSER_APPLICATIVE_HPP_
#define _PARSER_APPLICATIVE_HPP_

// Applicative implementation (mimicking Applicative type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   <*> → ^
//   <*  → <<
//   *>  → >>

#include <functional>

#include "abstractions/functor.hpp"
#include "helpers.hpp"

using namespace std;


// pure {{{1

template <template<typename>typename F, typename A>
// pure :: Applicative f => a -> f a
F<A> pure(A x)
{
	return pure<A>(x);
}

// }}}1


// apply {{{1

template <template<typename>typename F, typename A, typename B>
// (<*>) :: f (a → b) → f a → f b
F<B> apply(F<function<B(A)>> wrapped_fn, F<A> functor)
{
	return apply<A, B>(wrapped_fn, functor);
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “apply”
F<B> operator^(F<function<B(A)>> wrapped_fn, F<A> functor)
{
	return apply<F, A, B>(wrapped_fn, functor);
}

// }}}1


// apply arrows {{{1

// apply_first {{{2

template <template<typename>typename F, typename A, typename B>
// (<*) :: f a → f b → f a
F<A> apply_first(F<A> functor_a, F<B> functor_b)
{
	// (\a _ -> a) <$> functor_a <*> functor_b
	return apply<F, B, A>(
		fmap<F, A, function<A(B)>>(
			[](A a) { return [=](B) { return a; }; },
			functor_a
		),
		functor_b
	);
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “apply_first”
F<A> operator<<(F<A> functor_a, F<B> functor_b)
{
	return apply_first<F, A, B>(functor_a, functor_b);
}

// }}}2

// apply_second {{{2

template <template<typename>typename F, typename A, typename B>
// (*>) :: f a → f b → f b
F<B> apply_second(F<A> functor_a, F<B> functor_b)
{
	// (\_ b -> b) <$> functor_a <*> functor_b
	return apply<F, B, B>(
		fmap<F, A, function<B(B)>>(
			[](A) { return [](B b) { return b; }; },
			functor_a
		),
		functor_b
	);
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “apply_second”
F<B> operator>>(F<A> functor_a, F<B> functor_b)
{
	return apply_second<F, A, B>(functor_a, functor_b);
}

// }}}2

// }}}1

#endif
