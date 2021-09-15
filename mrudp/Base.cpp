#include "Base.h"

#include <iostream>

namespace timprepscius {
namespace mrudp {

#if defined(MRUDP_SINGLE_CHAR_TRACE)
#if 1

void xTraceChar_(void *self, uint32_t id, char c)
{
	std::ostringstream s;
	s << self << ": \t" << c << " " << id << std::endl;
	std::cerr << s.str();
}

void xTraceChar_(void *self, uint32_t id, char prefix, char c)
{
	std::ostringstream s;
	s << self << ": \t" << prefix << "(" << c << ")" << " " << id << std::endl;
	std::cerr << s.str();
}

#else

void xTraceChar_(void *self, uint32_t id, char c)
{
	std::cerr << c;
}

void xTraceChar_(void *self, uint32_t id, char prefix, char c)
{
	std::ostringstream s;
	s << prefix << "(" << c << ")";
	std::cerr << s.str();
}

void xTraceChar_(void *self, uint32_t id, char prefix, const std::string &c)
{
	std::ostringstream s;
	s << prefix << "(" << c << ")";
	std::cerr << s.str();
}
#endif
#endif

} // namespace
} // namespace
