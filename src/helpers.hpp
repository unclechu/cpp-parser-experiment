#pragma once

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

template <typename R, typename A>
// Idempotency
inline function<R(A)> curry(function<R(A)> fn)
{
	return fn;
}

template <typename R, typename A, typename B>
inline function<function<R(B)>(A)> curry(function<R(A, B)> fn)
{
	return [=](A a) { return [=](B b) { return fn(a, b); }; };
}

template <typename R, typename A, typename B, typename C>
inline function<function<function<R(C)>(B)>(A)> curry(function<R(A, B, C)> fn)
{
	return [=](A a) { return [=](B b) { return [=](C c) {
		return fn(a, b, c);
	}; }; };
}

template <typename R, typename A, typename B, typename C, typename D>
inline function<function<function<function<R(D)>(C)>(B)>(A)> curry(
	function<R(A, B, C, D)> fn
)
{
	return [=](A a) { return [=](B b) { return [=](C c) { return [=](D d) {
		return fn(a, b, c, d);
	}; }; }; };
}

template <typename R, typename A, typename B, typename C, typename D, typename E>
inline function<function<function<function<function<R(E)>(D)>(C)>(B)>(A)> curry(
	function<R(A, B, C, D, E)> fn
)
{
	return [=](A a) { return [=](B b) { return [=](C c) { return [=](D d) {
		return [=](E e) { return fn(a, b, c, d, e); };
	}; }; }; };
}

template <
	typename R,
	typename A,
	typename B,
	typename C,
	typename D,
	typename E,
	typename F
>
inline function<function<function<function<function<function<
	R(F)>(E)>(D)>(C)>(B)>(A)
> curry(
	function<R(A, B, C, D, E, F)> fn
)
{
	return [=](A a) { return [=](B b) { return [=](C c) { return [=](D d) {
		return [=](E e) { return [=](F f) { return fn(a, b, c, d, e, f); }; };
	}; }; }; };
}

// }}}1


// compose {{{1

template <typename R, typename A, typename B>
// (<<<) :: Category cat => cat b c -> cat a b -> cat a c
// Data.Function (.) :: (b -> c) -> (a -> b) -> a -> c
inline function<R(A)> compose(function<R(B)> f, function<B(A)> g)
{
	return [f,g](A a) -> R { return f(g(a)); };
}

template <typename R, typename A, typename B>
// Operator equivalent for “compose” function
inline function<R(A)> operator<(function<R(B)> f, function<B(A)> g)
{
	return compose<R, A, B>(f, g);
}

// }}}1
