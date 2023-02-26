#include "Common.h"

#include "../mrudp/mrudp_proxy.hpp"
#include "../mrudp/imp/Asio.h"

namespace timprepscius::mrudp::tests {

SCENARIO("proxy")
{
	auto numConnectionsToCreate = 256;
	auto numPacketsToSendOnEachConnection = 64;

    GIVEN( "mrudp service, remote socket" )
    {
		mrudp_options_asio_t options_;
		mrudp_default_options(MRUDP_IMP_ASIO, &options_);
		auto options_no_coalesce = options_;
		options_no_coalesce.connection.coalesce_reliable.mode = MRUDP_COALESCE_NONE;
		options_no_coalesce.connection.coalesce_unreliable.mode = MRUDP_COALESCE_NONE;

		auto options_coalesce_packet = options_;
		options_no_coalesce.connection.coalesce_reliable.mode = MRUDP_COALESCE_PACKET;
		options_no_coalesce.connection.coalesce_unreliable.mode = MRUDP_COALESCE_PACKET;

		auto options_coalesce_stream = options_;
		options_no_coalesce.connection.coalesce_reliable.mode = MRUDP_COALESCE_STREAM;
		options_no_coalesce.connection.coalesce_unreliable.mode = MRUDP_COALESCE_PACKET;
		
		List<std::tuple<String, mrudp_options_asio_t>> availableOptions = {
			{ "no coalesce", options_no_coalesce },
			{ "coalesce packet", options_coalesce_packet },
			{ "coalesce stream", options_coalesce_stream },
		};
		
		for (auto &[name, options]: availableOptions)
		{
			mrudp_addr_t anyAddress;
			mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
			
			WHEN(name)
			{
				auto proxyA_service = mrudp_service_ex(MRUDP_IMP_ASIO, &options);
				auto proxyB_service = mrudp_service_ex(MRUDP_IMP_ASIO, &options);

				mrudp_proxy_options_t proxyOptions = mrudp_proxy_options_default();
				
				mrudp_addr_t proxyA_address, proxyB_address;
				mrudp_str_to_addr("127.0.0.1:7654", &proxyA_address);
				mrudp_str_to_addr("127.0.0.1:4567", &proxyB_address);
				
				auto *proxyA = mrudp_proxy_open(proxyA_service, &proxyA_address, nullptr, &proxyA_address, &proxyOptions);
				auto *proxyB = mrudp_proxy_open(proxyB_service, &proxyB_address, &proxyA_address, &proxyB_address, &proxyOptions);
				
				core::ExecuteOnDestruct e1([=]() {
					sLogDebug("testing", "closing proxyA");
					mrudp_proxy_close(proxyA);
					sLogDebug("testing", "closing proxyB");
					mrudp_proxy_close(proxyB);
					
					std::this_thread::sleep_for(std::chrono::milliseconds(250));

					sLogDebug("testing", "closing proxyA service");
					mrudp_close_service(proxyA_service, true);
					sLogDebug("testing", "closing proxyB service");
					mrudp_close_service(proxyB_service, true);
				});
				
				{
					State remote("remote");
					remote.service = mrudp_service_ex(MRUDP_IMP_ASIO, &options);
					remote.sockets.push_back(mrudp_socket(remote.service, &anyAddress));

					mrudp_addr_t remoteAddress;
					mrudp_socket_addr(remote.sockets.back(), &remoteAddress);

					char remoteAddressStr[2048];
					mrudp_addr_to_str(&remoteAddress, remoteAddressStr, 2048);
					std::cout << "remote address is endpoint " << remoteAddressStr << std::endl;

					State local("local");
					local.service = mrudp_service_ex(MRUDP_IMP_ASIO, &options);
					
					auto listen = strong<Listener>(Listener {
						.accept = [&](auto connection) {
							auto l = lock_of(remote.connectionsMutex);
							remote.connections.insert(connection);
							
							auto remoteConnectionDispatch = strong<Connection>(Connection {
								.receive = [&](auto data, auto size, auto isReliable) {
									auto lock = lock_of(remote.packetsMutex);
									remote.packets.push_back(Packet(data, data+size));
									remote.packetsReceived++;
									remote.bytesReceived += size;
									return 0;
								},
								.close = [&remote, connection](auto event) {
									auto lock = lock_of(remote.connectionsMutex);
									auto connection_ = std::find(remote.connections.begin(),remote.connections.end(), connection);
									if (connection_ != remote.connections.end())
									{
										remote.connections.erase(connection);
										mrudp_close_connection(connection);
									}
									return 0;
								},
								.shouldDelete = false
							}) ;

							mrudp_accept(
								connection,
								ptr_of(remoteConnectionDispatch),
								connectionReceive,
								[remoteConnectionDispatch](void *p, auto e) { return connectionClose(p, e); }
							);
							
							return 0;
						},
						.close = [&](auto event) { return 0; }
					}) ;
					
					mrudp_listen(
						remote.sockets.back(),
						ptr_of(listen), nullptr,
						listenerAccept,
						[listen](void *p, auto e) { return listenerClose(p, e); }
					);
					
					WHEN("create one local socket and make many connections")
					{
						local.sockets.push_back(mrudp_socket(local.service, &anyAddress));
						mrudp_addr_t localAddress;
						mrudp_socket_addr(local.sockets.back(), &localAddress);
						
						auto localConnectionDispatch = strong<Connection>(Connection {
							.receive = [&](auto data, auto size, auto isReliable) {
								local.packetsReceived++;
								local.bytesReceived += size;
								return 0;
							},
							.close = [&](auto event) {
								return 0;
							}
						}) ;

						WHEN(numConnectionsToCreate << " connections are created from local to remote")
						{
							for (auto i=0; i<numConnectionsToCreate; ++i)
							{
								mrudp_connection_options_t options;
								options.coalesce_reliable.mode = MRUDP_COALESCE_PACKET;
								options.coalesce_reliable.delay_ms = 20;
								options.probe_delay_ms = -1;
								
								local.connections.insert(
									mrudp_connect_ex_proxy(
										local.sockets.back(), &remoteAddress,
										&options,
										ptr_of(localConnectionDispatch),
										connectionReceive,
										[localConnectionDispatch](void *p, auto e) { return connectionClose(p, e); },
										&proxyB_address, proxyOptions.magic_connection
									)
								);
							}
						
							wait_until(std::chrono::seconds(10), [&]() {
								return remote.connections.size() == numConnectionsToCreate;
							});
							
							auto numPacketsToSendOnEachConnection = 512;
							
							WHEN(numPacketsToSendOnEachConnection << " packets are sent in short bursts")
							{
								Packet packet = { 'a', 'b', 'c', 'd', 'e' };
								auto packetsSent = 0;
								auto bytesSent = 0;

								for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
								{
									if (i % 32 == 0)
										std::this_thread::sleep_for(std::chrono::milliseconds(100));
								
									for (auto &connection: local.connections)
									{
										mrudp_send(connection, packet.data(), (int)packet.size(), 1);
										packetsSent++;
										bytesSent += packet.size();
									}
								}
								
								wait_until(
									std::chrono::seconds(10),
									[&]() { return remote.bytesReceived == bytesSent; }
								);

								THEN("all data arrives")
								{
									REQUIRE(remote.bytesReceived == bytesSent);
								}

								if (options.connection.coalesce_reliable.mode != MRUDP_COALESCE_STREAM)
								{
									THEN("all packets arrive")
									{
										REQUIRE(remote.packetsReceived == packetsSent);
									}
								}

								if (options.connection.coalesce_reliable.mode != MRUDP_COALESCE_NONE)
								{
									THEN("the number of packets sent is far less than the number of data packets sent")
									{
										mrudp_connection_statistics_t statistics;
										mrudp_connection_statistics(*local.connections.begin(), &statistics);
										
										REQUIRE(statistics.reliable.packets.sent < statistics.reliable.frames.sent / 16);
									}
								}
							}
						}
					}
					
					WHEN("create one local socket and make many connections")
					{
						local.sockets.push_back(mrudp_socket(local.service, &anyAddress));
						mrudp_addr_t localAddress;
						mrudp_socket_addr(local.sockets.back(), &localAddress);
						
						auto localConnectionDispatch = strong<Connection>(Connection {
							.receive = [&](auto data, auto size, auto isReliable) {
								local.packetsReceived++;
								local.packets.push_back(Packet(data, data+size));
								local.bytesReceived += size;
								return 0;
							},
							.close = [&](auto event) {
								return 0;
							}
						}) ;

						NO_WHEN(numConnectionsToCreate << " connections are created from local to remote")
						{
							for (auto i=0; i<numConnectionsToCreate; ++i)
							{
								local.connections.insert(
									mrudp_connect_proxy(
										local.sockets.back(), &remoteAddress,
										ptr_of(localConnectionDispatch),
										connectionReceive,
										[localConnectionDispatch](void *p, auto e) { return connectionClose(p, e); },
										&proxyB_address, proxyOptions.magic_connection
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
							
							WHEN("the local connections are closed")
							{
								while (!local.connections.empty())
								{
									auto front = local.connections.begin();
									mrudp_close_connection(*front);
									local.connections.erase(front);
								}

								THEN("connections on remote return to 0")
								{
									wait_until(std::chrono::seconds(MRUDP_MAXIMUM_CONNECTION_TIMEOUT), [&]() {
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
									auto front = local.connections.begin();
									mrudp_close_connection(*front);
									local.connections.erase(front);
								}

								THEN("connections on remote return to 0")
								{
									wait_until(std::chrono::seconds(MRUDP_MAXIMUM_CONNECTION_TIMEOUT), [&]() {
										return remote.connections.size() == 0;
									});
								
									REQUIRE(remote.connections.size() == 0);
								}
							}
						}
						
						WHEN(numConnectionsToCreate << " connections are created and " << numPacketsToSendOnEachConnection << " packets are sent on each")
						{
							auto packetsSent = 0;
							auto bytesSent = 0;
							
							for (auto i=0; i<numConnectionsToCreate; ++i)
							{
								local.connections.insert(mrudp_connect_proxy(
									local.sockets.back(), &remoteAddress,
									ptr_of(localConnectionDispatch),
									connectionReceive,
									[localConnectionDispatch](void *p, auto e) { return connectionClose(p, e); },
									&proxyB_address, proxyOptions.magic_connection
								));
							}

							Packet packet = { 'a', 'b', 'c', 'd', 'e' };

							for (auto &connection: local.connections)
							{
								for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
								{
									mrudp_send(connection, packet.data(), (int)packet.size(), 1);
									packetsSent++;
									bytesSent += packet.size();
								}
							}
							
							if (options.connection.coalesce_reliable.mode != MRUDP_COALESCE_STREAM)
							{
								THEN(numPacketsToSendOnEachConnection << " packets show up and are correct")
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

							THEN("data shows up")
							{
								wait_until(
									std::chrono::seconds(100),
									[&]() { return remote.bytesReceived == bytesSent; }
								);

								REQUIRE(remote.bytesReceived == bytesSent);
							}
							
							WHEN(numPacketsToSendOnEachConnection << " packets are sent back oppositely")
							{
							
								auto numPacketsSentRemoteToLocal = 0;
								auto numBytesSentRemoteToLocal = 0;
								
								wait_until(std::chrono::seconds(10), [&]() {
									return remote.connections.size() == numConnectionsToCreate;
								});
								
								for (auto &connection: remote.connections)
								{
									for (auto i=0; i<numPacketsToSendOnEachConnection; ++i)
									{
										mrudp_send(connection, packet.data(), (int)packet.size(), 1);
										numPacketsSentRemoteToLocal++;
										numBytesSentRemoteToLocal += packet.size();
									}
								}
								
								if (options.connection.coalesce_reliable.mode != MRUDP_COALESCE_STREAM)
								{
									THEN("packets show up and are correct")
									{
										wait_until(
											std::chrono::seconds(10),
											[&]() { return local.packetsReceived == numPacketsSentRemoteToLocal; }
										);

										REQUIRE(local.packetsReceived == packetsSent);
										
										auto l = lock_of(local.packetsMutex);
										
										bool allMatch = std::all_of(
											local.packets.begin(), local.packets.end(),
											[&packet](auto &v) { return v == packet; }
										);
										
										REQUIRE(allMatch);
									}
								}

								THEN("data shows up")
								{
									wait_until(
										std::chrono::seconds(10),
										[&]() { return local.bytesReceived == numBytesSentRemoteToLocal; }
									);

									REQUIRE(local.bytesReceived == numBytesSentRemoteToLocal);
								}
							}
							
							WHEN("the local connections are closed")
							{
								wait_until(std::chrono::seconds(10), [&]() {
									return remote.connections.size() == numConnectionsToCreate;
								});

								{
									auto l = lock_of(remote.connectionsMutex);
									REQUIRE(remote.connections.size() == numConnectionsToCreate);
								}

								while (!local.connections.empty())
								{
									auto front = local.connections.begin();
									mrudp_close_connection(*front);
									local.connections.erase(front);
								}

								THEN("connections on remote return to 0")
								{
									wait_until(std::chrono::seconds(MRUDP_MAXIMUM_CONNECTION_TIMEOUT * 2), [&]() {
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
									auto front = local.connections.begin();
									mrudp_close_connection(*front);
									local.connections.erase(front);
								}

								THEN("connections on remote return to 0")
								{
									wait_until(std::chrono::seconds(MRUDP_MAXIMUM_CONNECTION_TIMEOUT), [&]() {
										return remote.connections.size() == 0;
									});
								
									REQUIRE(remote.connections.size() == 0);
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
