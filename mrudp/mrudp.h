#pragma once

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

// The maximum data payload size can be sent.
// 1500 - sizeof(Header) - sanity = 1400
#define MRUDP_MAX_PACKET_SIZE 1400
#define MRUDP_OK 0
#define MRUDP_GENERAL_FAILURE -1

// The events possible for when a close callback is invoked
typedef enum {
	MRUDP_EVENT_NOT_ACCEPTED,
	MRUDP_EVENT_TIMEOUT,
	MRUDP_EVENT_CLOSED,
} mrudp_event_t;

// The result error code returned by all functions
typedef int mrudp_error_code_t;

// A useful macro to check if a function failed
#define mrudp_failed(x) ( (x) != MRUDP_OK )

// An anonymous structure for the service handle
typedef struct { int silence_warnings; } mrudp_service_t_;
typedef mrudp_service_t_ *mrudp_service_t;

// An anonymous structure for the socket handle
typedef struct { int silence_warnings; } mrudp_socket_t_;
typedef mrudp_socket_t_ *mrudp_socket_t;

// An anonymous structure for the connection handle
typedef struct { int silence_warnings; } mrudp_connection_t_;
typedef mrudp_connection_t_ *mrudp_connection_t;

// The address structure
typedef union {
    struct sockaddr ip;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
} mrudp_addr_t;

// Call-back for when a handle is about to be closed
typedef mrudp_error_code_t (*mrudp_close_callback_fn)(void *, mrudp_event_t);

// Call-back for when a connection is being accepted
typedef mrudp_error_code_t (*mrudp_accept_callback_fn)(void *, mrudp_connection_t connection);

// Call-back for when data has been received
typedef mrudp_error_code_t (*mrudp_receive_callback_fn)(void *, char *data, int size, int is_reliable);

// Call-back for when an address has been resolved
typedef mrudp_error_code_t (*mrudp_resolve_callback_fn)(void *, const mrudp_addr_t *addresses, size_t numAddresses);

// converts an ip+port string into an address
mrudp_error_code_t mrudp_str_to_addr(const char *str, mrudp_addr_t *addr);

// converts an address into an ip+port string
mrudp_error_code_t mrudp_addr_to_str(const mrudp_addr_t *addr, char *out, size_t outSize);

// creates a service
mrudp_service_t mrudp_service ();

// closes a service
void mrudp_close_service(mrudp_service_t mrudp, int waitForFinish);

// resolves an address ip string to an address, on complete or error, the resolve callback is invoked
mrudp_error_code_t mrudp_resolve(mrudp_service_t mrudp, const char *address, mrudp_resolve_callback_fn, void *userData);
 
 // creates a socket using the given service and address
mrudp_socket_t mrudp_socket(mrudp_service_t service, mrudp_addr_t *address);

// connects a socket to a remote address (this is to test throughput for connected sockets on linux)
mrudp_error_code_t mrudp_socket_connect(mrudp_socket_t socket, mrudp_addr_t *remote);

// closes a given socket, after closing the socket handle is invalid
mrudp_error_code_t mrudp_close_socket(mrudp_socket_t socket);

// closes the native handle for the given socket immediately
mrudp_error_code_t mrudp_close_socket_native(mrudp_socket_t socket);

// enables listening for a given socket,
// the accept call-back is invoked when a new connection is preliminarly established
// the close call-back is invoked when the socket is closed
mrudp_error_code_t mrudp_listen(
	mrudp_socket_t socket,
	void *userData,
	mrudp_accept_callback_fn,
	mrudp_close_callback_fn
) ;

// accepts a given connection, setting the receive and close callback
// the receive call-back is invoked when data is received
// the close call-back is invoked when the connection is closed
mrudp_error_code_t mrudp_accept(
	mrudp_connection_t connection,
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn
);

// gets the local address of the given socket
mrudp_error_code_t mrudp_socket_addr(mrudp_socket_t socket, mrudp_addr_t *);

// generates a connection using the given socket, to the given address
// the receive call-back is invoked when data is received
// the close call-back is invoked when the connection is closed
mrudp_connection_t mrudp_connect(
	mrudp_socket_t socket,
	const mrudp_addr_t *,
	
	void *userData,
	mrudp_receive_callback_fn,
	mrudp_close_callback_fn
);

// gets the remote address of the given connection
mrudp_error_code_t mrudp_connection_remote_addr(mrudp_connection_t connection, mrudp_addr_t *);

// closes a connection
mrudp_error_code_t mrudp_close_connection(mrudp_connection_t connection);

// closes the native handle for the socket of the given connection immediately
mrudp_error_code_t mrudp_close_connection_socket_native(mrudp_connection_t socket);

// sends data on the given connection
mrudp_error_code_t mrudp_send (mrudp_connection_t connection, const char *, int size, int reliable);

#ifdef __cplusplus
}
#endif
