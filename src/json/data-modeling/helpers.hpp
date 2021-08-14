#pragma once

#include <functional>

#include "../../helpers.hpp"

using namespace std;


// Make a “constructor” function out of any “struct”.
// “Constructor” in terms of Haskell ADTs (Algebraic Data Types).
//
// This helps to write applicative parsers for complex data-types.
// Just like in Haskell.
//
// Imagine in Haskell you have this:
//
//   data Person = Person
//     { name :: String
//     , age :: Word
//     }
//
// You would write a parser like this:
//
//   Person
//     <$> parseName
//     <*> parseAge
//
// In C++ you would describe the type like this:
//
//   struct Person {
//     string name;
//     uint8_t age;
//   };
//
// And a parser like this (using this helper):
//
//   curry<Person, string, uint8_t>(constructor<Person, string, uint8_t>)
//     ^ parse_name()
//     ^ parse_age()
//
template <typename T, typename ... Args>
inline T constructor(Args... fields)
{
	T x { fields... };
	return x;
}

// curry_constructor {{{1

// Helps to avoid specifying template arguments twice, like this:
//
//   curry<Person, string, uint8_t>(constructor<Person, string, uint8_t>)
//
// Instead you just do it like this:
//
//   curry_constructor<Person, string, uint8_t>

template <typename R, typename A>
inline function<R(A)> curry_constructor()
{
	return curry<R, A>(constructor<R, A>);
}

template <typename R, typename A, typename B>
inline function<function<R(B)>(A)> curry_constructor()
{
	return curry<R, A, B>(constructor<R, A, B>);
}

template <typename R, typename A, typename B, typename C>
inline function<function<function<R(C)>(B)>(A)> curry_constructor()
{
	return curry<R, A, B, C>(constructor<R, A, B, C>);
}

template <typename R, typename A, typename B, typename C, typename D>
inline function<function<function<function<R(D)>(C)>(B)>(A)> curry_constructor()
{
	return curry<R, A, B, C, D>(constructor<R, A, B, C, D>);
}

template <typename R, typename A, typename B, typename C, typename D, typename E>
inline function<function<function<function<function<
	R(E)>(D)>(C)>(B)>(A)
> curry_constructor()
{
	return curry<R, A, B, C, D, E>(constructor<R, A, B, C, D, E>);
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
> curry_constructor()
{
	return curry<R, A, B, C, D, E, F>(constructor<R, A, B, C, D, E, F>);
}

// }}}1
