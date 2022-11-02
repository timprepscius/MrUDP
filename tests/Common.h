
#include "../mrudp/mrudp.h"

#include <mutex>
#include <list>
#include <set>
#include <atomic>
#include <thread>
#include <chrono>

#include "catch.hpp"

#include "../mrudp/base/Core.h"
#include "../mrudp/base/Mutex.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

typedef std::function<int(mrudp_connection_t)> ListenerAcceptFunction;
typedef std::function<int(mrudp_event_t)> ListenerCloseFunction;

typedef std::function<int(const char *, int, int)> ConnectionReceiveFunction;
typedef std::function<int(mrudp_event_t)> ConnectionCloseFunction;

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::duration<Clock> Duration;

struct Listener {
	ListenerAcceptFunction accept;
	ListenerCloseFunction close;
	bool shouldDelete = false;
} ;

struct Connection {
	ConnectionReceiveFunction receive;
	ConnectionCloseFunction close;
	bool shouldDelete = false;
} ;

inline
int listenerAccept(void *l_, mrudp_connection_t connection)
{
	auto l = reinterpret_cast<Listener *>(l_);
	return l->accept(connection);
}

inline
int listenerClose(void *l_, mrudp_event_t event)
{
	auto l = reinterpret_cast<Listener *>(l_);
	int result = MRUDP_OK;
	
	if (l->close)
		result = l->close(event);
	
	if (l->shouldDelete)
		delete l;
	
	return result;
}

inline
int connectionReceive(void *l_, char *data, int size, int isReliable)
{
	auto l = reinterpret_cast<Connection *>(l_);
	return l->receive(data, size, isReliable);
}

inline
int connectionClose(void *l_, mrudp_event_t event)
{
	auto l = reinterpret_cast<Connection *>(l_);
	int result = MRUDP_OK;
	
	if (l->close)
		result = l->close(event);
	
	if (l->shouldDelete)
		delete l;
	
	return result;
}

typedef std::vector<char> Packet;

struct State {
	std::string label;

	mrudp_service_t service;
	std::list<mrudp_socket_t> sockets;
	
	Mutex connectionsMutex;
	std::set<mrudp_connection_t> connections;
	
	Mutex packetsMutex;
	std::list<Packet> packets;
	
	std::atomic<size_t> packetsReceived = 0;
	std::atomic<size_t> bytesReceived = 0;
	
	State(const std::string &label_ = "") :
		label(label_)
	{
	}
	
	~State ()
	{
		sLogDebug("mrudp::life_cycle", label << " state destructing");
	
		{
			for (auto &socket : sockets)
				mrudp_close_socket(socket);
			sockets.clear();
		}

		{
			decltype(connections) connections_;
			
			{
				auto lock = lock_of(connectionsMutex);
				connections_ = std::move(connections);
			}
			
			for (auto &connection: connections_)
				mrudp_close_connection(connection);
		}

		{
			mrudp_close_service(service, true);
			service = nullptr;
		}

		sLogDebug("mrudp::life_cycle", label << " state destructing finished");
	}
} ;

inline
void wait_a_bit()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
}

template<typename D, typename F>
inline auto wait_until(D &&d, F &&f)
{
	auto then = Clock::now();
	
	auto expire = then + d;
	while (!f())
	{
		auto now = Clock::now();
		if (now > expire)
		{
			break;
		}
			
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	auto now = Clock::now();
	return now - then;
}

#define NO_WHEN(...) if (false)
#define NO_GIVEN(...) if (false)

} // namespace
} // namespace
} // namespace
