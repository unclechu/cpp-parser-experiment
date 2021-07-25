#ifndef _HELPERS_HPP_
#define _HELPERS_HPP_

#include <functional>
#include <optional>
#include <ostream>
#include <tuple>

using namespace std;


struct Unit: tuple<> {};
Unit unit();
ostream& operator<<(ostream &out, Unit&);


// Helpers for pattern-matching-ish syntax for std::visit taken from here:
// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


template <typename A>
inline function<A(A)> const_map(A x)
{
	return [=](A) { return x; };
}

template <typename A>
inline optional<A> to_optional(A x)
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


template <template<typename...>typename T>
inline string chars_to_string(T<char> list)
{
	string str(list.begin(), list.end());
	return str;
}


// curry {{{1

template <typename A, typename B>
// Idempotency
function<B(A)> curry(function<B(A)> fn)
{
	return fn;
}

template <typename A, typename B, typename C>
function<function<C(B)>(A)> curry(function<C(A, B)> fn)
{
	return [=](A a) { return [=](B b) { return fn(a, b); }; };
}

template <typename A, typename B, typename C, typename D>
function<function<function<D(C)>(B)>(A)> curry(function<D(A, B, C)> fn)
{
	return [=](A a) { return [=](B b) { return [=](C c) {
		return fn(a, b, c);
	}; }; };
}

template <typename A, typename B, typename C, typename D, typename E>
function<function<function<function<E(D)>(C)>(B)>(A)> curry(
	function<E(A, B, C, D)> fn
)
{
	return [=](A a) { return [=](B b) { return [=](C c) { return [=](C d) {
		return fn(a, b, c, d);
	}; }; }; };
}

// }}}1


// compose {{{1

template <typename A, typename B, typename C>
// (<<<) :: Category cat => cat b c -> cat a b -> cat a c
// Data.Function (.) :: (b -> c) -> (a -> b) -> a -> c
inline function<C(A)> compose(function<C(B)> f, function<B(A)> g)
{
	return [f,g](A a) -> C { return f(g(a)); };
}

template <typename A, typename B, typename C>
// Operator equivalent for “compose” function
inline function<C(A)> operator<(function<C(B)> f, function<B(A)> g)
{
	return compose<A, B, C>(f, g);
}

// }}}1

#endif
