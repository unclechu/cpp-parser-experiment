#include <ostream>
#include <tuple>

#include "helpers.hpp"


Unit unit()
{
	return Unit{std::make_tuple()};
}

std::ostream& operator<<(std::ostream &out, Unit&)
{
	return out << "Unit{}";
}
