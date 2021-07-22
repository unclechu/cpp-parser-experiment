#ifndef _PARSER_ALTERNATIVE_HPP_
#define _PARSER_ALTERNATIVE_HPP_

// Alternative implementation (mimicking Alternative type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   fail → fail_parser (“fail” is occupied by STL)

#include <functional>
#include <variant>
#include <vector>

#include "helpers.hpp"
#include "parser/helpers.hpp"
#include "parser/types.hpp"

using namespace std;


// alt {{{1

template <typename A>
// (<|>) :: Alternative f => f a -> f a -> f a
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
// Operator equivalent for “alt”
Parser<A> operator||(Parser<A> a, Parser<A> b)
{
	return alt<A>(a, b);
}

// }}}1

// some {{{1

template <typename A>
// One or more.
// some :: Alternative f => f a -> f [a]
// WARNING! Do not apply on parsers that are always successful (like “pure(…)”)
// The recursion will never end in this case. It doesn’t mean you can’t compose
// “pure” with parsers that are used with this function. The only point is that
// parser must fail at some point to finalize the resulting list.
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

	return map_parsing_failure<vector<A>>(
		[](ParsingError err) {
			return ParsingError{
				"‘some’: failed to parse at least one single element: " + err
			};
		},
		Parser<vector<A>>{parser_fn}
	);
}

// }}}1

// many {{{1

template <typename A>
// Zero or more.
// many :: Alternative f => f a -> f [a]
// WARNING! Do not apply on parsers that are always successful (like “pure(…)”)
// The recursion will never end in this case. It doesn’t mean you can’t compose
// “pure” with parsers that are used with this function. The only point is that
// parser must fail at some point to finalize the resulting list.
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

#endif
