#ifndef _HELPERS_HPP_
#define _HELPERS_HPP_

#include <functional>
#include <optional>
#include <tuple>

struct Unit: std::tuple<> {};

// Helpers for pattern-matching-ish syntax for std::visit taken from here:
// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename A>
inline std::function<A(A)> const_map(A x)
{
	return [=](A) { return x; };
}

template <typename A>
inline std::optional<A> to_optional(A x)
{
	return x;
};

template <typename A>
inline A id(A x)
{
	return x;
}

template <typename A>
inline A negative(A x)
{
	return -x;
}

#endif
