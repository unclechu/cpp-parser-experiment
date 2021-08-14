#pragma once

// “Parser” type definition and a couple of tightly related types.
//
// “Parser” type is mimicking of how one would do this in Haskell
// (see “attoparsec” library for instance, its “Parser” is implemented in a
// more complicated way but on the high level it looks kind of the same).

#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "../helpers.hpp"
#include "abstractions/monadfail.hpp"

using namespace std;


template <typename I>
struct ParsingError: pair<string, I> {};

template <typename A, typename I>
struct ParsingSuccess: pair<A, I> {};

template <typename A, typename I>
using ParsingResult = variant<ParsingError<I>, ParsingSuccess<A, I>>;

template <typename A>
// Parser a = String → Either String (a, String)
struct Parser: function<ParsingResult<A, string>(string)> {};


template <template<typename>typename F>
struct ParserInput;

// Less verbose abstraction on top of “ParserInput”
template <template<typename>typename F>
using ParserInputType = decltype(ParserInput<F>::input_type);


// Instance for “Parser”
template <>
struct ParserInput<Parser> { string input_type; };


// Generic type class instances-ish for all parser types {{{1

template <typename I>
inline ParsingError<I> make_parsing_error(string message, I input);

template <typename A, typename I>
inline ParsingSuccess<A, I> make_parsing_success(A value, I input);

template <typename A, template<typename>typename F>
inline F<A> prefix_parsing_failure(string pfx, F<A> parser);

// MonadFail
template <typename A = Unit, template<typename>typename F>
F<A> fail(string err)
{
	using I = ParserInputType<F>;
	return F<A>{[=](I input) { return make_parsing_error<I>(err, input); }};
}

// Functor
template <typename A, typename B, template<typename>typename F>
F<B> fmap(function<B(A)> map_fn, F<A> parser)
{
	using I = ParserInputType<F>;
	return F<B>{[=](I input) {
		return visit(overloaded {
			[=](ParsingSuccess<A, I> x) -> ParsingResult<B, I> {
				auto [ value, tail ] = x;
				return make_parsing_success<B, I>(map_fn(value), tail);
			},
			[](ParsingError<I> err) -> ParsingResult<B, I> { return err; },
		}, parser(input));
	}};
}

// Applicative
template <typename A, template<typename>typename F>
F<A> pure(A x)
{
	using I = ParserInputType<F>;
	return F<A>{[x](I input) { return make_parsing_success<A, I>(x, input); }};
}

// Applicative
template <typename A, typename B, template<typename>typename F>
F<B> apply(F<function<B(A)>> fn_parser, F<A> parser)
{
	using I = ParserInputType<F>;
	return F<B>{[=](I input) -> ParsingResult<B, I> {
		return visit(overloaded {
			[parser](ParsingSuccess<function<B(A)>, I> x) -> ParsingResult<B, I> {
				auto [ fn, tail ] = x;
				return fmap<A, B, F>(fn, parser)(tail);
			},
			[](ParsingError<I> err) -> ParsingResult<B, I> { return err; }
		}, fn_parser(input));
	}};
}

// Alternative
template <typename A, template<typename>typename F>
F<A> alt(F<A> parser_a, F<A> parser_b)
{
	using I = ParserInputType<F>;
	return F<A>{[=](I input) {
		return visit(overloaded {
			[input, parser_b](ParsingError<I>) -> ParsingResult<A, I> {
				return parser_b(input);
			},
			[](ParsingSuccess<A, I> x) -> ParsingResult<A, I> { return x; }
		}, parser_a(input));
	}};
}

// Alternative
template <typename A, template<typename>typename F>
F<vector<A>> some(F<A> parser)
{
	using I = ParserInputType<F>;
	function<ParsingResult<vector<A>, I>(I)> parser_fn = [=](I input) {
		return visit(overloaded {
			[](ParsingError<I> err) -> ParsingResult<vector<A>, I> {
				return err;
			},
			[parser](ParsingSuccess<A, I> first) -> ParsingResult<vector<A>, I> {
				auto [ first_element, first_tail ] = first;
				vector<A> list = { first_element };

				using RecurResult = ParsingSuccess<vector<A>, I>;
				using RecurFn = function<RecurResult(I)>;
				RecurFn recur = [&recur, &list, parser](I tail) {
					return visit(overloaded {
						[&list, tail](ParsingError<I>) -> RecurResult {
							return make_parsing_success<vector<A>, I>(list, tail);
						},
						[&list, &recur](
							ParsingSuccess<A, I> current
						) -> RecurResult {
							auto [ current_element, current_tail ] = current;
							list.push_back(current_element);
							return recur(current_tail);
						}
					}, parser(tail));
				};

				return recur(first_tail);
			}
		}, parser(input));
	};

	return prefix_parsing_failure<vector<A>, F>(
		"some: failed to parse at least one single element",
		F<vector<A>>{parser_fn}
	);
}

// Alternative
template <typename A, template<typename>typename F>
F<vector<A>> many(F<A> parser)
{
	using I = ParserInputType<F>;
	return F<vector<A>>{[=](I input) {
		vector<A> list;

		using RecurResult = ParsingSuccess<vector<A>, I>;
		using RecurFn = function<RecurResult(I)>;
		RecurFn recur = [&recur, &list, parser](I tail) {
			return visit(overloaded {
				[&list, tail](ParsingError<I>) -> RecurResult {
					return make_parsing_success<vector<A>, I>(list, tail);
				},
				[&list, &recur](ParsingSuccess<A, I> current) -> RecurResult {
					auto [ current_element, current_tail ] = current;
					list.push_back(current_element);
					return recur(current_tail);
				}
			}, parser(tail));
		};

		return recur(input);
	}};
}

// }}}1


// Type class instances-ish for “Parser” type {{{1

// MonadFail
template <typename A = Unit>
inline Parser<A> fail(string err)
{
	return fail<A, Parser>(err);
}

// Functor
template <typename A, typename B>
inline Parser<B> fmap(function<B(A)> map_fn, Parser<A> parser)
{
	return fmap<A, B, Parser>(map_fn, parser);
}

// Applicative
template <typename A>
inline Parser<A> pure(A x)
{
	return pure<A, Parser>(x);
}

// Applicative
template <typename A, typename B>
inline Parser<B> apply(Parser<function<B(A)>> fn_parser, Parser<A> parser)
{
	return apply<A, B, Parser>(fn_parser, parser);
}

// Alternative
template <typename A>
inline Parser<A> alt(Parser<A> parser_a, Parser<A> parser_b)
{
	return alt<A, Parser>(parser_a, parser_b);
}

// Alternative
template <typename A>
inline Parser<vector<A>> some(Parser<A> parser)
{
	return some<A, Parser>(parser);
}

// Alternative
template <typename A>
inline Parser<vector<A>> many(Parser<A> parser)
{
	return many<A, Parser>(parser);
}

template <typename A>
inline Parser<A> prefix_parsing_failure(string pfx, Parser<A> parser)
{
	return prefix_parsing_failure<A, Parser>(pfx, parser);
}

template <typename A, template<typename>typename F>
inline F<vector<A>> optional_list(F<vector<A>> parser);

template <typename A>
inline Parser<vector<A>> optional_list(Parser<vector<A>> parser)
{
	return optional_list<A, Parser>(parser);
}

// }}}1


// Helpers {{{1

template <typename I>
inline ParsingError<I> make_parsing_error(string message, I input)
{
	return ParsingError<I>{make_pair(message, input)};
}

template <typename A, typename I>
inline ParsingSuccess<A, I> make_parsing_success(A value, I input)
{
	return ParsingSuccess<A, I>{make_pair(value, input)};
}

template <typename A, template<typename>typename F>
F<A> map_parsing_failure(
	function<
		ParsingError<ParserInputType<F>>(ParsingError<ParserInputType<F>>)
	> map_fn,
	F<A> parser
)
{
	using I = ParserInputType<F>;
	return F<A>{[=](I input) {
		return visit(overloaded {
			[map_fn](ParsingError<I> err) -> ParsingResult<A, I> {
				return map_fn(err);
			},
			[](ParsingSuccess<A, I> x) -> ParsingResult<A, I> { return x; }
		}, parser(input));
	}};
}

template <typename A, template<typename>typename F>
inline F<A> prefix_parsing_failure(string pfx, F<A> parser)
{
	return map_parsing_failure<A>(
		[pfx](ParsingError<ParserInputType<F>> err) {
			return make_parsing_error<ParserInputType<F>>(
				pfx + ": " + err.first,
				err.second
			);
		},
		parser
	);
}

template <typename A, template<typename>typename F>
// Resolved to empty list by default
inline F<vector<A>> optional_list(F<vector<A>> parser)
{
	vector<A> empty_list;
	return parser || pure<decltype(empty_list), F>(empty_list);
}

// }}}1
