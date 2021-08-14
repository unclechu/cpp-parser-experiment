#pragma once

#include <functional>
#include <type_traits>
#include <vector>

#include "json/types.hpp"

using namespace std;


template <typename T>
JsonValue to_json(T);


template <
	template<typename...>typename F,
	typename A,
	enable_if_t<is_same_v<F<A>, vector<A>>, bool> = true
>
JsonValue to_json(F<A> list)
{
	if constexpr (is_same_v<F<A>, vector<JsonValue>>) {
		return to_json(list);
	} else {
		vector<JsonValue> arr;
		for (auto x : list) arr.push_back(to_json(x));
		return to_json(arr);
	}
}
