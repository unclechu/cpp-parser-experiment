#ifndef _HELPERS_HPP_
#define _HELPERS_HPP_

#include <tuple>

struct Unit: std::tuple<> {};

// Helpers for pattern-matching-ish syntax for std::visit taken from here:
// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif
