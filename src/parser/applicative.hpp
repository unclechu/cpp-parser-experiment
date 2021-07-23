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
// pure :: Applicative f => a -> f a
// pure :: Applicative f => a -> Parser a
Parser<A> pure(A x)
{
	return Parser<A>{[=](Input input) { return make_tuple(x, input); }};
}

// }}}1


// apply {{{1

template <typename A, typename B>
// (<*>) :: f (a → b) → f a → f b
// (<*>) :: Parser (a → b) → Parser a → Parser b
Parser<B> apply(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return Parser<B>{[=](Input input) -> ParsingResult<B> {
		return visit(overloaded {
			[=](ParsingSuccess<function<B(A)>> x) -> ParsingResult<B> {
				auto [ fn, tail ] = x;
				return fmap<A, B>(fn, parser)(tail);
			},
			[](ParsingError err) -> ParsingResult<B> { return err; }
		}, fn_parser(input));
	}};
}

template <template<typename>typename F, typename A, typename B>
// Operator equivalent for “apply”
F<B> operator^(F<function<B(A)>> wrapped_fn, F<A> functor)
{
	return apply<A, B>(wrapped_fn, functor);
}

// }}}1


// apply arrows {{{1

// apply_first {{{2

template <template<typename>typename F, typename A, typename B>
// (<*) :: f a → f b → f a
F<A> apply_first(F<A> functor_a, F<B> functor_b)
{
	// (\a _ -> a) <$> functor_a <*> functor_b
	return apply<B, A>(
		fmap<A, function<A(B)>>(
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
	return apply<B, B>(
		fmap<A, function<B(B)>>(
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
