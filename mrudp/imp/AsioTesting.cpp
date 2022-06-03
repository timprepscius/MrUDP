#include "AsioTesting.h"

#ifdef MRUDP_INTRUSIVE_TEST

namespace timprepscius {
namespace mrudp {
namespace imp {

AsioIntrusiveError &__mrudp_asio_getIntrusiveError()
{
	static AsioIntrusiveError v;
	return v;
}

Vector<boost::system::error_code *> __mrudp_asio_getIntrusiveErrors()
{
	auto &v = __mrudp_asio_getIntrusiveError();
	
	Vector<boost::system::error_code *> result = {
		&v.getConnectedSocket__connectedSocket_handle_open,
		&v.getConnectedSocket__connectedSocket_handle_set_option,
		&v.getConnectedSocket__connectedSocket_handle_bind,
		
		&v.acquireAddress__socket_handle_close,
		&v.acquireAddress__socket_handle_open,
		&v.acquireAddress__socket_handle_bind,
		&v.acquireAddress__socket_handle_open_overlapped,
		&v.acquireAddress__socket_handle_set_option_overlapped,
		&v.acquireAddress__socket_handle_bind_overlapped,
		&v.acquireAddress__temporary_close_overlapped
	};
	
	return result;
}

} // namespace
} // namespace
} // namespace

#endif
