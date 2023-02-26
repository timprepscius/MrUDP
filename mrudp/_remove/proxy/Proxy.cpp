#include "ProxyID.h"
#include "Proxy.h"
#include "../Base.h"

#include "../imp/Asio.h"
#include "../base/Core.h"

namespace timprepscius {
namespace mrudp {
namespace proxy {

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::system;
using namespace core;

struct ProxyIDs {

	ProxyID firstProxyID = 1;
	ProxyID lastProxyID = std::numeric_limits<ProxyID>().max();

	OrderedMap<udp::endpoint, ProxyID> forwards;
	OrderedMap<ProxyID, udp::endpoint> backwards;

	const udp::endpoint *get(const ProxyID &id)
	{
		auto i = backwards.find(id);
		if (i != backwards.end())
			return &i->second;
			
		return nullptr;
	}

	ProxyID get(const udp::endpoint &endpoint)
	{
		auto i = forwards.find(endpoint);
		if (i != forwards.end())
			return i->second;
			
		std::cout << this << " new endpoint " << endpoint.address().to_string() << " : " << endpoint.port() << std::endl;
		
		ProxyID found = 0;

		if (backwards.empty())
			found = 1;
			
		if (!found)
		{
			auto last = backwards.rbegin()->first;
			if (last < lastProxyID)
				found = last+1;
		}
		
		if (!found)
		{
			auto first = backwards.begin()->first;
			if (first > firstProxyID)
				found = first-1;
		}
		
		if (!found)
		{
			auto last = firstProxyID;
			for (auto &[id, endpoint]: backwards)
			{
				if (id > last+1)
				{
					found = last+1;
					break;
				}
			}
		}
		
		if (!found)
			return 0;
			
		backwards[found] = endpoint;
		forwards[endpoint] = found;
		
		return found;
	}
} ;

void proxy(udp::socket &handle, bool dynamicTo, const udp::endpoint &to_, bool *end, ProxyMagic magic)
{

	std::cout << "proxying on endpoint " << handle.local_endpoint() << std::endl;
	
	udp::endpoint from;
	udp::endpoint to = to_;
	
	ProxyIDs proxyIDs;
	proxy::Transformer proxy;
	
	Vector<char> buffer_(4092);
	auto size = buffer_.size();
	
	while (!*end)
	{
		error_code error;
		
		auto bytesTransferred = handle.receive_from(
			buffer(buffer_, size),
			from, 0,
			error
		);
		
		if (!error)
		{
			auto *packet = (Packet *)buffer_.data();
			packet->dataSize = bytesTransferred - sizeof(Header);
			
			ProxyID proxyID = 0;
			if (proxy.receiveBackward(*packet, proxyID))
			{
				if (auto to = proxyIDs.get(proxyID))
				{
					handle.send_to(
						buffer((char *)packet, packet->dataSize + sizeof(Header)),
						*to
					);
					
					std::cerr << "<";
				}
				else
				{
					std::cerr << "there is no connection for " << proxyID << std::endl;
				}
			}
			else
			if (proxy.receiveMagic(*packet, magic))
			{
				if (dynamicTo)
				{
					to = from;
					
					std::cerr << "magic signal accepted " << from << std::endl;
				}
				else
				{
					std::cerr << "magic ignored " << from << std::endl;
				
				}
			}
			else
			{
				auto proxyID = proxyIDs.get(from);
				if (proxyID)
				{
					proxy.sendForward(*packet, proxyID);
					
					handle.send_to(
						buffer((char *)packet, packet->dataSize + sizeof(Header)),
						to
					);
					
					std::cerr << ">";
				}
				else
				{
					std::cerr << "there is no proxy id for " << from << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "there was an error " << error << std::endl;
		}
	}
}

void send_connect(mrudp_socket_t socket_, const mrudp_addr_t *to, mrudp_proxy_magic_t magic)
{
	auto socket = toNative(socket_);
	if (!socket)
		return ;
	
	imp::Send send;
	send.address = *to;
	send.packet = strong<Packet>();
	
	proxy::Transformer proxy;
	proxy.sendMagic(*send.packet, magic);
	socket->imp->socket->send(send, [](auto e){});
}

struct ProxyMemento {
	io_service service;
	udp::socket *socket;
	udp::endpoint from, to, bound;
	bool dynamicTo;
	mrudp_proxy_magic_t magic;

	std::thread thread;
	bool end;
} ;

void *open(const mrudp_addr_t *from, const mrudp_addr_t *to, mrudp_addr_t *bound, mrudp_proxy_magic_t magic)
{
	auto memento = new ProxyMemento;
	
	memento->from = imp::toEndpoint(*from);
	
	if (to)
		memento->to = imp::toEndpoint(*to);
	
	memento->dynamicTo = to == nullptr;
	memento->magic = magic;

	
	memento->socket = new udp::socket(memento->service);
	auto &socket = *memento->socket;
	socket.open(memento->from.protocol());
	socket.bind(memento->from);
	
	memento->bound = socket.local_endpoint();
	*bound = imp::toAddr(memento->bound);
	
	memento->thread = std::thread([memento]() {
		proxy(*memento->socket, memento->dynamicTo, memento->to, &memento->end, memento->magic);
	});
	
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	
	std::cout << "opened endpoint " << memento->bound << std::endl;

	return memento;
}

void close (void *memento_)
{
	auto *memento = (ProxyMemento *)memento_;
	std::cout << "closing endpoint " << memento->bound << std::endl;

	memento->end = true;
	memento->service.stop();
	memento->socket->close();
	
	if (memento->thread.joinable())
		memento->thread.join();
	
	delete memento->socket;
	delete memento;
}

} // namespace
} // namespace
} // namespace
