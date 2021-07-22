#pragma once

#include "Connection.h"
#include "Socket.h"
#include "Service.h"

#include <uvw/uvw.hpp>

namespace timprepscius {
namespace mrudp {

struct ServiceImp : StrongThis<ServiceImp>
{
	Service *parent;
	std::shared_ptr<uvw::Loop> loop;

	std::thread runner;
	std::recursive_mutex mutex;
	
	bool running = false;

	ServiceImp (Service *parent_) :
		parent(parent_)
	{
		loop = uvw::Loop::create();
	}
	
	~ServiceImp ()
	{
	
	}
	
	void start ()
	{
		running = true;
		runner = std::thread([this]() {
			while (running)
			{
				std::lock_guard<std::recursive_mutex> lock(mutex);
				loop->run<uvw::Loop::Mode::NOWAIT>();
			}
		});
	}
	
	void stop ()
	{
		if (!running)
			return;
			
		running = false;
		loop->stop();
		
		if (runner.joinable())
			runner.join();
	}
	
	void lock()
	{
		mutex.lock();
	}
	
	void unlock()
	{
		mutex.unlock();
	}
} ;

struct ConnectionImp
{
	Connection *parent;
	std::shared_ptr<uvw::TimerHandle> timer;

	ConnectionImp(Connection *parent_) :
		parent(parent_)
	{
		xLogDebug(logOfThis(this) << logOfThis(parent));

		timer = parent->socket->service->imp->loop->resource<uvw::TimerHandle>();

		timer->on<uvw::TimerEvent>([this](const auto &, auto &handle) {
			handle.stop();
			onTimeout();
		});
	}
	
	~ConnectionImp()
	{
		xLogDebug(logOfThis(this) << logOfThis(parent));
		stop();
	}
	
	void stop ()
	{
		xLogDebug(logOfThis(this) << logOfThis(parent));
		timer->close();
	}

	Function<void()> timeoutFunction;

	void setTimeout (const Timepoint &then, Function<void()> &&f)
	{
		xLogDebug(logOfThis(this) << logOfThis(parent));

		auto durationFromNow = then - parent->socket->service->clock.now();
		auto durationInMS = std::chrono::duration_cast<std::chrono::milliseconds>(durationFromNow).count();
	
			xLogDebug(logOfThis(this) << logOfThis(parent) << logLabel("setting timeout") << logVar(durationInMS));
	
		timeoutFunction = std::move(f);
	
		timer->stop();
		timer->start(uvw::TimerHandle::Time{durationInMS}, uvw::TimerHandle::Time{0});
	}
	
	void onTimeout ()
	{
		xLogDebug(logOfThis(this) << logOfThis(parent));

		timeoutFunction();
	}
} ;

struct SocketImp
{
	Socket *parent;

	std::shared_ptr<uvw::UDPHandle> handle;

	SocketImp(Socket *parent_, const Address &address) :
		parent(parent_)
	{
		handle = parent->service->imp->loop->resource<uvw::UDPHandle>();
		handle->bind(String("127.0.0.1"), ntohs(address.sin_port));
	}
	
	void open ()
	{
		handle->on<uvw::UDPDataEvent>([this](const uvw::UDPDataEvent &event, uvw::UDPHandle &handle) {
				// @TODO: set up bound checking
				Packet packet;
				memcpy(&packet, event.data.get(), event.length);
			parent->receive(packet, event.sender);
		});
		
		handle->recv();
	}
	
	void send (const mrudp_addr_t &addr, char *data, size_t size)
	{
		char *mem = new char[size];
		memcpy(mem, data, size);
		handle->send(*(sockaddr *)&addr, mem, size);
	}
	
	void close ()
	{
		handle->close();
	}
	
	mrudp_addr_t getLocalAddress ()
	{
		mrudp_addr_t addr;
		int namelen = sizeof(addr);
		uv_udp_getsockname(handle->raw(), (sockaddr *)&addr, &namelen);
		return addr;
	}
	
} ;

} // namespace
} // namespace
