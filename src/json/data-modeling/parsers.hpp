#pragma once

#include <functional>
#include <iomanip>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../../helpers.hpp"
#include "json/data-modeling/types.hpp"

using namespace std;


// FromJSON type-class-ish definition
//
// This mimics like if you had this in Haskell:
//
//   class FromJson a where
//     fromJson :: JsonValue -> Parser a
//
template <typename T>
FromJsonParser<T> from_json();

template <
	template<typename...>typename F,
	typename A,
	enable_if_t<is_same_v<F<A>, vector<A>>, bool> = true
>
FromJsonParser<F<A>> from_json(FromJsonParser<A> item_parser)
{
	using T = F<A>;
	using I = ParserInputType<FromJsonParser>;
	return FromJsonParser<T>{[item_parser](I input) {
		using R = ParsingResult<T, I>;
		return visit(overloaded {
			[](ParsingError<I> err) -> R { return err; },
			[input, item_parser](ParsingSuccess<vector<JsonValue>, I> x) -> R {
				T mapped_list;

				for (size_t i = 0; i < x.first.size(); ++i) {
					optional<ParsingError<I>> failure = nullopt;

					list<string> json_path = x.second.second;
					json_path.push_back("[" + to_string(i) + "]");
					I item_input = make_pair(x.first[i], json_path);

					visit(overloaded {
						[&failure](ParsingError<I> err) {
							failure = make_parsing_error<I>(
								// TODO print type that was targered to be parsed
								"Failed to parse JsonArray item: " + err.first,
								err.second
							);
						},
						[&mapped_list](ParsingSuccess<A, I> y) {
							mapped_list.push_back(y.first);
						}
					}, item_parser(item_input));

					if (failure != nullopt)
						return failure.value();
				}

				return make_parsing_success<T, I>(mapped_list, input);
			}
		}, from_json<vector<JsonValue>>()(input));
	}};
}

template <typename T>
FromJsonParser<T> in_key(string k, FromJsonParser<T> parser)
{
	using I = ParserInputType<FromJsonParser>;
	using M = map<string, JsonValue>;
	using R = ParsingResult<T, I>;

	ostringstream prefix;
	prefix << "in_key(" << quoted(k) << ")";

	auto override_input = [](I to_input, R result) -> R {
		return visit(overloaded {
			[](ParsingError<I> err) -> R { return err; },
			[to_input](ParsingSuccess<T, I> x) -> R {
				return make_parsing_success<T, I>(x.first, to_input);
			}
		}, result);
	};

	return prefix_parsing_failure<T>(
		prefix.str(),
		FromJsonParser<T>{[k, parser, override_input](I input) {
			return visit(overloaded {
				[](ParsingError<I> err) -> R { return err; },
				[k, parser, input, override_input](ParsingSuccess<M, I> x) -> R {
					M::iterator it = x.first.find(k);
					if (it == x.first.end()) {
						ostringstream err_msg;
						err_msg << "Key " << quoted(k) << " is not found";
						return make_parsing_error<I>(err_msg.str(), x.second);
					} else {
						JsonPath new_path = x.second.second;
						new_path.push_back(k);
						return override_input(
							input,
							parser(make_pair(it->second, new_path))
						);
					}
				}
			}, from_json<M>()(input));
		}}
	);
}
