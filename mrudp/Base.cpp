#include "Base.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

void trace_char_(char c)
{
	std::cout << c;
}


void trace_char_(const String &c)
{
	std::cout << c;
}

} // namespace
} // namespace
