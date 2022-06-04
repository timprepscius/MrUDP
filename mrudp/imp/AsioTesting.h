#pragma once

#include "../Base.h"

//#define MRUDP_INTRUSIVE_TEST

#ifdef MRUDP_INTRUSIVE_TEST
#include "BoostAsio.h"
#endif

namespace timprepscius {
namespace mrudp {
namespace imp {

#ifdef MRUDP_INTRUSIVE_TEST

struct AsioIntrusiveError
{
	boost::system::error_code
		getOverlappedSocket__overlappedSocket_handle_open,
		getOverlappedSocket__overlappedSocket_handle_set_option,
		getOverlappedSocket__overlappedSocket_handle_bind,
		
		acquireAddress__socket_handle_close,
		acquireAddress__socket_handle_open,
		acquireAddress__socket_handle_bind,
		acquireAddress__socket_handle_open_overlapped,
		acquireAddress__socket_handle_set_option_overlapped,
		acquireAddress__socket_handle_bind_overlapped,
		acquireAddress__temporary_close_overlapped;
} ;

AsioIntrusiveError &__mrudp_asio_getIntrusiveError();
Vector<boost::system::error_code *> __mrudp_asio_getIntrusiveErrors();

#define MRUDP_ASIO_TEST_GENERATE_FAILURE(x, y) \
	{ \
		auto &e = timprepscius::mrudp::imp::__mrudp_asio_getIntrusiveError().x; \
		if (e) \
			y = e; \
	} \
	
#else

#define MRUDP_ASIO_TEST_GENERATE_FAILURE(...)

#endif

} // namespace
} // namespace
} // namespace

