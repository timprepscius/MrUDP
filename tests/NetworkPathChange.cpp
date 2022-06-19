
#include "../mrudp/mrudp.h"

#include "Common.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

#define NO_THEN(...) if (false)

SCENARIO("network path change")
{
	auto numConnectionsToCreate = 1;
	size_t numPacketsToSendOnEachConnection = 16;
	
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
				auto l = lock_of(remote.connectionsMutex);
				remote.connections.insert(connection);
				
				auto remoteConnectionDispatch = new Connection {
					.receive = [&](auto data, auto size, auto isReliable) {
						auto lock = lock_of(remote.packetsMutex);
						remote.packets.push_back(Packet(data, data+size));
						remote.packetsReceived++;
						return 0;
					},
					.close = [&remote, connection](auto event) {
						auto lock = lock_of(remote.connectionsMutex);
						auto connection_ = std::find(remote.connections.begin(),remote.connections.end(), connection);
						if (connection_ != remote.connections.end())
						{
							mrudp_close_connection(connection);
							remote.connections.erase(connection_);
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
			
			auto localConnectionDispatch = Connection {
				[&](auto data, auto size, auto isReliable) {
					return 0;
				},
				[&](auto event) {
					return 0;
				}
			} ;

			WHEN(numConnectionsToCreate << " connections are created from local to remote and then destroyed")
			{
				for (auto i=0; i<numConnectionsToCreate; ++i)
				{
					local.connections.insert(
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
					auto l = lock_of(remote.connectionsMutex);
					REQUIRE(remote.connections.size() == numConnectionsToCreate);
				}
				
				WHEN(numPacketsToSendOnEachConnection << " packets are sent on each")
				{
					size_t packetsSent = 0;
					
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
						
						WHEN("client sockets moves to new port")
						{
							{
								// todo should lock a different mutex?
								auto l = lock_of(local.connectionsMutex);
								for (auto &socket: local.sockets)
								{
									mrudp_relocate_socket(socket, &anyAddress);
								}
							}
							
							WHEN("packets are sent again")
							{
								// clear out variables
								{
									auto l = lock_of(remote.packetsMutex);
									remote.packets.clear();
									remote.packetsReceived = 0;
								}
								
								for (auto &connection: local.connections)
								{
									for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
									{
										mrudp_send(connection, packet.data(), (int)packet.size(), 1);
										packetsSent++;
									}
								}
								
								NO_THEN("packets show up and are correct")
								{
									wait_until(
										std::chrono::seconds(10),
										[&]() { return remote.packetsReceived == packetsSent; }
									);

									REQUIRE(remote.packetsReceived == packetsSent);
									
									auto l = lock_of(remote.packetsMutex);
									
									bool allMatch = std::all_of(
										remote.packets.begin(), remote.packets.end(),
										[&packet](auto &v) { return v == packet; }
									);
									
									REQUIRE(allMatch);
								}
							
							}
						}

						WHEN("server socket moves to another port")
						{
							{
								// todo should lock a different mutex?
								auto l = lock_of(remote.connectionsMutex);
								for (auto &socket: remote.sockets)
								{
									mrudp_relocate_socket(socket, &anyAddress);
								}
							}
							
							WHEN("packets are sent again")
							{
								// clear out variables
								{
									auto l = lock_of(remote.packetsMutex);
									remote.packets.clear();
									remote.packetsReceived = 0;
									packetsSent = 0;
								}
								
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
									
									auto l = lock_of(remote.packetsMutex);
									
									bool allMatch = std::all_of(
										remote.packets.begin(), remote.packets.end(),
										[&packet](auto &v) { return v == packet; }
									);
									
									REQUIRE(allMatch);
								}
							
							}
						}
					}

				}
			}
		}
	}
}

} // namespace
} // namespace
} // namespace
