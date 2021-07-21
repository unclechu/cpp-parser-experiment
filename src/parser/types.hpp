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

#endif
