#include "Base.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

#ifdef MRUDP_SINGLE_CHAR_TRACE

void xTraceChar_(char c)
{
	std::cout << c;
}

void xTraceChar_(const String &c)
{
	std::cout << c;
}

#endif

} // namespace
} // namespace
