#ifndef _ABSTRACTIONS_MONADFAIL_HPP_
#define _ABSTRACTIONS_MONADFAIL_HPP_

// MonadFail implementation (mimicking MonadFail type class from Haskell)
//
// Definitions in relation to Haskell (Haskell version on the left):
//   fail → fail
//
// Minimal implementaion for any new F type:
//   fail

#include <string>

#include "helpers.hpp"

using namespace std;


template <template<typename>typename F, typename A = Unit>
F<A> fail(string err)
{
	return fail<A>(err);
}

#endif
