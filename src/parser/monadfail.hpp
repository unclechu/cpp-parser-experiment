#ifndef _PARSER_MONADFAIL_HPP_
#define _PARSER_MONADFAIL_HPP_

// MonadFail implementation (mimicking MonadFail type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   fail → fail_parser (“fail” is occupied by STL)

#include <string>

#include "helpers.hpp"
#include "parser/types.hpp"

using namespace std;


template <typename A = Unit>
Parser<A> fail_parser(string err)
{
	return Parser<A>{[=](Input) { return ParsingError{err}; }};
}

#endif
