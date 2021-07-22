
#include "../mrudp/mrudp.h"

#include "Common.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("mrudp packet transmission rate")
{
	auto X = 256;
	auto Y = 128;

//	LogActivateStory(LogAllStories);
//	LogActivateStory("mrudp::retry");
//	LogActivateStory("mrudp::life_cycle");

    GIVEN( "mrudp service, remote socket" )
    {
		mrudp_addr_t anyAddress;
		mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
		
		State remote("remote");
		remote.service = mrudp_service();
		remote.sockets.push_back(mrudp_socket(remote.service, &anyAddress));
		
		mrudp_addr_t remoteAddress;
		mrudp_socket_addr(remote.sockets.back(), &remoteAddress);
;
		State local("local");
		local.service = mrudp_service();
		
		auto connectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				return remote.packetsReceived++;
			},
			[&](auto event) { return 0; }
		};
		
		auto listenerDispatch = Listener {
			[&](auto connection) {
				auto l = Lock(remote.connectionsMutex);
				remote.connections.push_back(connection);
				
				mrudp_accept(
					connection,
					&connectionDispatch,
					connectionReceive,
					connectionClose
				);
				return 0;
			},
			[&](auto event) {
				return 0;
			}
		} ;
		
		mrudp_listen(
			remote.sockets.back(),
			&listenerDispatch, listenerAccept, listenerClose
		);
		
		WHEN("create a local socket with X conections to the remote socket")
		{
			local.sockets.push_back(mrudp_socket(local.service, &anyAddress));
			
			auto localConnectionDispatch = Connection {
				[&](auto data, auto size, auto isReliable) {
					return 0;
				},
				[&](auto event) {
					return 0;
				}
			} ;
			
			for (auto i=0; i<X; ++i)
			{
				local.connections.push_back(
					mrudp_connect(
						local.sockets.back(), &remoteAddress,
						&localConnectionDispatch,
						connectionReceive,
						connectionClose
					)
				);
			}
				
			char packet[] = { 'a', 'b', 'c', 'd', 'e' };
			
			size_t packetsSent = 0;
			
			THEN("send Y packets of data on each connection")
			{
				auto then = Clock::now();
			
				for (auto &connection: local.connections)
				{
					for (auto i=0; i<Y; ++i)
					{
						mrudp_send(connection, packet, sizeof(packet), 1);
						packetsSent++;
					}
				}
				
				wait_until(std::chrono::seconds(10), [&]() { return remote.packetsReceived == packetsSent; });
				
				auto now = Clock::now();
				auto duration = now - then;
				
				REQUIRE(remote.packetsReceived == packetsSent);
				
				auto durationInMS = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
				auto durationInS = durationInMS.count() / 1000.0;
				auto packetsPerSecond = packetsSent / durationInS;
				
				auto requiredPacketsPerSecond = 50000.0;
				REQUIRE(packetsPerSecond > requiredPacketsPerSecond);
			}
		}

		WHEN("create X local sockets and make one connections on each to the single remote")
		{
			mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
		
			auto localConnectionDispatch = Connection {
				[&](auto data, auto size, auto isReliable) {
					return 0;
				},
				[&](auto event) { return 0; }
			} ;
		
			for (auto i=0; i<X; ++i)
			{
				local.sockets.push_back(mrudp_socket(local.service, &anyAddress));
			
				local.connections.push_back(
					mrudp_connect(
						local.sockets.back(), &remoteAddress,
						&localConnectionDispatch,
						connectionReceive,
						connectionClose
					)
				);
			}
			
			char packet[] = { 'a', 'b', 'c', 'd', 'e' };
			
			size_t packetsSent = 0;
			
			THEN("send Y packets of data on each connection")
			{
				auto then = Clock::now();
			
				for (auto &connection: local.connections)
				{
					for (auto i=0; i<Y; ++i)
					{
						mrudp_send(connection, packet, sizeof(packet), 1);
						packetsSent++;
					}
				}
				
				wait_until(std::chrono::seconds(10), [&]() { return remote.packetsReceived == packetsSent; });
				
				auto now = Clock::now();
				auto duration = now - then;
				
				REQUIRE(remote.packetsReceived == packetsSent);
				
				auto durationInMS = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
				auto durationInS = durationInMS.count() / 1000.0;
				auto packetsPerSecond = packetsSent / durationInS;
				
				auto requiredPacketsPerSecond = 50000.0;
				REQUIRE(packetsPerSecond > requiredPacketsPerSecond);
			}
		}
	}
	
    GIVEN( "mrudp service, remote and local sockets paired" )
    {
		mrudp_addr_t anyAddress;
		mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
		
		State remote("remote");
		remote.service = mrudp_service();
		
		State local("local");
		local.service = mrudp_service();
		
		auto remoteConnectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				return remote.packetsReceived++;
			},
			[&](auto event) { return 0; }
		} ;

		auto localConnectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				return 0;
			},
			[&](auto event) { return 0; }
		} ;
		
		auto listen = Listener {
			[&](auto connection) {
				auto l = Lock(remote.connectionsMutex);
				remote.connections.push_back(connection);
				
				mrudp_accept(connection,
					&remoteConnectionDispatch,
					connectionReceive,
					connectionClose
				);
				return 0;
			},
			[&](auto event) {
				return 0;
			}
		} ;
		
		WHEN ("establish connections")
		{
			for (auto i=0; i<X; ++i)
			{
				mrudp_addr_t remoteAddress;
				auto remoteSocket = mrudp_socket(remote.service, &anyAddress);
				mrudp_socket_addr(remoteSocket, &remoteAddress);
				remote.sockets.push_back(remoteSocket);
					
				mrudp_listen(remoteSocket, &listen, listenerAccept, listenerClose);
			
				mrudp_addr_t localAddress;
				auto localSocket = mrudp_socket(local.service, &anyAddress);
				mrudp_socket_addr(localSocket, &localAddress);
				local.sockets.push_back(localSocket);
				
//				mrudp_socket_connect(remoteSocket, &localAddress);
//				mrudp_socket_connect(localSocket, &remoteAddress);
			
				local.connections.push_back(mrudp_connect(
					localSocket, &remoteAddress,
					&localConnectionDispatch,
					connectionReceive, connectionClose
				));
			}
		
			char packet[] = { 'a', 'b', 'c', 'd', 'e' };
			
			size_t packetsSent = 0;
			
			THEN("send Y packets of data on each connection")
			{
				auto then = Clock::now();
			
				for (auto &connection: local.connections)
				{
					for (auto i=0; i<Y; ++i)
					{
						mrudp_send(connection, packet, sizeof(packet), 1);
						packetsSent++;
					}
				}
				
				auto expire = then + std::chrono::seconds(10);
				while (remote.packetsReceived != packetsSent)
				{
					auto now = Clock::now();
					if (now > expire)
						break;
						
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				
				auto now = Clock::now();
				auto duration = now - then;
				
				REQUIRE(remote.packetsReceived == packetsSent);
				
				auto durationInMS = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
				auto durationInS = durationInMS.count() / 1000.0;
				auto packetsPerSecond = packetsSent / durationInS;
				
				auto requiredPacketsPerSecond = 50000.0;
				REQUIRE(packetsPerSecond > requiredPacketsPerSecond);
			}
		}
	}
	
}

} // namespace
} // namespace
} // namespace
