
#include "../mrudp/mrudp.h"

#include "Common.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("basics")
{
//	xLogActivateStory(xLogAllStories);
	auto numConnectionsToCreate = 256;
	auto numPacketsToSendOnEachConnection = 64;

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
		
		auto listen = Listener {
			.accept = [&](auto connection) {
				auto l = Lock(remote.connectionsMutex);
				remote.connections.push_back(connection);
				
				auto remoteConnectionDispatch = new Connection {
					.receive = [&](auto data, auto size, auto isReliable) {
						Lock lock(remote.packetsMutex);
						remote.packets.push_back(Packet(data, data+size));
						remote.packetsReceived++;
						return 0;
					},
					.close = [&remote, connection](auto event) {
						Lock lock(remote.connectionsMutex);
						auto connection_ = std::find(remote.connections.begin(),remote.connections.end(), connection);
						if (connection_ != remote.connections.end())
						{
							remote.connections.remove(connection);
							mrudp_close_connection(connection);
						}
						return 0;
					},
					true
				} ;

				mrudp_accept(
					connection,
					remoteConnectionDispatch,
					connectionReceive,
					connectionClose
				);
				
				return 0;
			},
			.close = [&](auto event) { return 0; }
		} ;
		
		mrudp_listen(remote.sockets.back(), &listen, listenerAccept, listenerClose);
		
		WHEN("create one local socket and make many connections")
		{
			local.sockets.push_back(mrudp_socket(local.service, &anyAddress));
			mrudp_addr_t localAddress;
			mrudp_socket_addr(local.sockets.back(), &localAddress);
			
			mrudp_socket_connect(local.sockets.back(), &remoteAddress);
			mrudp_socket_connect(remote.sockets.back(), &localAddress);
			
			auto localConnectionDispatch = Connection {
				.receive = [&](auto data, auto size, auto isReliable) {
					local.packetsReceived++;
					local.packets.push_back(Packet(data, data+size));
					return 0;
				},
				.close = [&](auto event) {
					return 0;
				}
			} ;

			NO_WHEN("X connections are created from local to remote")
			{
				for (auto i=0; i<numConnectionsToCreate; ++i)
				{
					local.connections.push_back(
						mrudp_connect(
							local.sockets.back(), &remoteAddress,
							&localConnectionDispatch, connectionReceive, connectionClose
						)
					);
				}
			
				wait_until(std::chrono::seconds(10), [&]() {
					return remote.connections.size() == numConnectionsToCreate;
				});

				THEN("connections show up on the remote")
				{
					Lock l(remote.connectionsMutex);
					REQUIRE(remote.connections.size() == numConnectionsToCreate);
				}
				
				WHEN("the local connections are closed")
				{
					while (!local.connections.empty())
					{
						mrudp_close_connection(local.connections.back());
						local.connections.pop_back();
					}

					THEN("connections on remote return to 0")
					{
						wait_until(std::chrono::seconds(10), [&]() {
							return remote.connections.size() == 0;
						});
					
						REQUIRE(remote.connections.size() == 0);
					}
				}
				
				WHEN("the local socket is close_native immediately")
				{
					for (auto &socket : local.sockets)
					{
						mrudp_close_socket_native(socket);
					}
					
					while (!local.connections.empty())
					{
						mrudp_close_connection(local.connections.back());
						local.connections.pop_back();
					}

					THEN("connections on remote return to 0")
					{
						wait_until(std::chrono::seconds(30), [&]() {
							return remote.connections.size() == 0;
						});
					
						REQUIRE(remote.connections.size() == 0);
					}
				}
			}
			
			WHEN("X connections are created and Y packets are sent on each")
			{
				auto packetsSent = 0;
				
				for (auto i=0; i<numConnectionsToCreate; ++i)
				{
					local.connections.push_back(mrudp_connect(
						local.sockets.back(), &remoteAddress,
						&localConnectionDispatch, connectionReceive, connectionClose
					));
				}

				Packet packet = { 'a', 'b', 'c', 'd', 'e' };

				for (auto &connection: local.connections)
				{
					for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
					{
						mrudp_send(connection, packet.data(), (int)packet.size(), 1);
						packetsSent++;
					}
				}
				
				THEN("packets show up and are correct")
				{
					wait_until(
						std::chrono::seconds(10),
						[&]() { return remote.packetsReceived == packetsSent; }
					);

					REQUIRE(remote.packetsReceived == packetsSent);
					
					Lock l(remote.packetsMutex);
					
					bool allMatch = std::all_of(
						remote.packets.begin(), remote.packets.end(),
						[&packet](auto &v) { return v == packet; }
					);
					
					REQUIRE(allMatch);
				}
				
				WHEN("packets are sent back oppositely")
				{
				
					auto numPacketsSentRemoteToLocal = 0;
					
					wait_until(std::chrono::seconds(10), [&]() {
						return remote.connections.size() == numConnectionsToCreate;
					});
					
					for (auto &connection: remote.connections)
					{
						for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
						{
							mrudp_send(connection, packet.data(), (int)packet.size(), 1);
							numPacketsSentRemoteToLocal++;
						}
					}
					
					THEN("packets show up and are correct")
					{
						wait_until(
							std::chrono::seconds(10),
							[&]() { return local.packetsReceived == numPacketsSentRemoteToLocal; }
						);

						REQUIRE(local.packetsReceived == packetsSent);
						
						Lock l(local.packetsMutex);
						
						bool allMatch = std::all_of(
							local.packets.begin(), local.packets.end(),
							[&packet](auto &v) { return v == packet; }
						);
						
						REQUIRE(allMatch);
					}
				}
				
				WHEN("the local connections are closed")
				{
					wait_until(std::chrono::seconds(10), [&]() {
						return remote.connections.size() == numConnectionsToCreate;
					});

					{
						Lock l(remote.connectionsMutex);
						REQUIRE(remote.connections.size() == numConnectionsToCreate);
					}

					while (!local.connections.empty())
					{
						mrudp_close_connection(local.connections.back());
						local.connections.pop_back();
					}

					THEN("connections on remote return to 0")
					{
						wait_until(std::chrono::seconds(10), [&]() {
							return remote.connections.size() == 0;
						});
					
						REQUIRE(remote.connections.size() == 0);
					}
				}
				
				WHEN("the local socket is close_native immediately")
				{
					for (auto &socket : local.sockets)
					{
						mrudp_close_socket_native(socket);
					}
					
					while (!local.connections.empty())
					{
						mrudp_close_connection(local.connections.back());
						local.connections.pop_back();
					}

					THEN("connections on remote return to 0")
					{
						wait_until(std::chrono::seconds(30), [&]() {
							return remote.connections.size() == 0;
						});
					
						REQUIRE(remote.connections.size() == 0);
					}
				}
			}
		}
	}
}

} // namespace
} // namespace
} // namespace
