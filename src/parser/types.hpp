#ifndef _PARSER_TYPES_HPP_
#define _PARSER_TYPES_HPP_

// “Parser” type definition and a couple of tightly related types.
//
// “Parser” type is mimicking of how one would do this in Haskell
// (see “attoparsec” library for instance, its “Parser” is implemented in a
// more complicated way but on the high level it looks kind of the same).

#include <functional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "../helpers.hpp"
#include "abstractions/monadfail.hpp"

using namespace std;


using Input = string;
struct ParsingError: string {};

template <typename A>
using ParsingSuccess = tuple<A, Input>;

template <typename A>
using ParsingResult = variant<ParsingError, ParsingSuccess<A>>;

template <typename A>
// Parser a = String → Either String (a, String)
struct Parser: function<ParsingResult<A>(Input)> {};


// Type class instances-ish for “Parser” type {{{1

// MonadFail
template <typename A = Unit>
Parser<A> fail(string err)
{
	return Parser<A>{[=](Input) { return ParsingError{err}; }};
}

// Functor
template <typename A, typename B>
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

// Applicative
template <typename A>
Parser<A> pure(A x)
{
	return Parser<A>{[=](Input input) { return make_tuple(x, input); }};
}

// Applicative
template <typename A, typename B>
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

// Alternative
template <typename A>
Parser<A> alt(Parser<A> parser_a, Parser<A> parser_b)
{
	return Parser<A>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingError) -> ParsingResult<A> { return parser_b(input); },
			[](ParsingSuccess<A> x) -> ParsingResult<A> { return x; }
		}, parser_a(input));
	}};
}

template <typename A>
Parser<A> prefix_parsing_failure(string pfx, Parser<A> parser);

// Alternative
template <typename A>
Parser<vector<A>> some(Parser<A> parser)
{
	function<ParsingResult<vector<A>>(Input)> parser_fn = [=](Input input) {
		return visit(overloaded {
			[](ParsingError err) -> ParsingResult<vector<A>> { return err; },
			[parser](ParsingSuccess<A> first) -> ParsingResult<vector<A>> {
				auto [ first_element, first_tail ] = first;
				vector<A> list = { first_element };

				using RecurResult = ParsingSuccess<vector<A>>;
				using RecurFn = function<RecurResult(Input)>;
				RecurFn recur = [&recur, &list, parser](Input tail) {
					return visit(overloaded {
						[&list, tail](ParsingError) -> RecurResult {
							return make_tuple(list, tail);
						},
						[&list, &recur](ParsingSuccess<A> current) -> RecurResult {
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

	return prefix_parsing_failure<vector<A>>(
		"some: failed to parse at least one single element",
		Parser<vector<A>>{parser_fn}
	);
}

// Alternative
template <typename A>
Parser<vector<A>> many(Parser<A> parser)
{
	return Parser<vector<A>>{[=](Input input) {
		vector<A> list;

		using RecurResult = ParsingSuccess<vector<A>>;
		using RecurFn = function<RecurResult(Input)>;
		RecurFn recur = [&recur, &list, parser](Input tail) {
			return visit(overloaded {
				[&list, tail](ParsingError) -> RecurResult {
					return make_tuple(list, tail);
				},
				[&list, &recur](ParsingSuccess<A> current) -> RecurResult {
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


// Helpers {{{1

template <typename A>
Parser<A> map_parsing_failure(
	function<ParsingError(ParsingError)> map_fn,
	Parser<A> parser
)
{
	return Parser<A>{[=](Input input) {
		return visit(overloaded {
			[=](ParsingError err) -> ParsingResult<A> { return map_fn(err); },
			[](ParsingSuccess<A> x) -> ParsingResult<A> { return x; }
		}, parser(input));
	}};
}

template <typename A>
Parser<A> prefix_parsing_failure(string pfx, Parser<A> parser)
{
	return map_parsing_failure<A>(
		[pfx](ParsingError err) { return ParsingError{pfx + ": " + err}; },
		parser
	);
}

// }}}1

#endif
