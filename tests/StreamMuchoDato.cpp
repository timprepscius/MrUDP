
#include "../mrudp/mrudp.h"

#include <iostream>
#include "Common.h"


namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("stream_lots")
{
	xLogActivateStory("mrudp::ack_failure");
//	xLogActivateStory("mrudp::rtt_computation");

    GIVEN( "mrudp service, remote and local sockets paired" )
    {
		mrudp_addr_t anyAddress;
		mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
		
		State remote("remote");
		remote.service = mrudp_service();
		
		State local("local");
		local.service = mrudp_service();
		
		u8 nextRemoteReceive = 0;
		u8 nextLocalReceive = 0;
		u8 nextRemoteSend = 0;
		u8 nextLocalSend = 0;
		
		auto remoteConnectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				for (auto *ptr = (u8 *)data; ptr != (u8 *)data + size; ++ptr)
				{
					debug_assert(nextRemoteReceive == *ptr);
					nextRemoteReceive++;
				}
				
				remote.bytesReceived += size;
				remote.packetsReceived++;
				return MRUDP_OK;
			},
			[&](auto event) { return 0; }
		} ;

		auto localConnectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				for (auto i=0; i<size; ++i)
				{
					debug_assert(nextLocalReceive == data[i]);
					nextLocalReceive++;
				}

				local.bytesReceived += size;
				return local.packetsReceived++;
			},
			[&](auto event) { return 0; }
		} ;
		
		auto listen = Listener {
			[&](auto connection) {
				auto l = lock_of(remote.connectionsMutex);
				remote.connections.insert(connection);
				
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
		
		WHEN ("establish a connections")
		{
			mrudp_addr_t remoteAddress;
			auto remoteSocket = mrudp_socket(remote.service, &anyAddress);
			mrudp_socket_addr(remoteSocket, &remoteAddress);
			remote.sockets.push_back(remoteSocket);
				
			mrudp_listen(remoteSocket, &listen, nullptr, listenerAccept, listenerClose);
		
			mrudp_addr_t localAddress;
			auto localSocket = mrudp_socket(local.service, &anyAddress);
			mrudp_socket_addr(localSocket, &localAddress);
			local.sockets.push_back(localSocket);
			
			auto localConnection_ = local.connections.insert(mrudp_connect(
				localSocket, &remoteAddress,
				&localConnectionDispatch,
				connectionReceive, connectionClose
			));
			
			auto &localConnection = *localConnection_.first;
		
			WHEN("send the stream packets of data on the connection")
			{
				size_t packetsSent = 0;
				size_t bytesSent = 0;
				
				const auto maxPacketSize = 512;
				const auto maxDelay = 0;
				const auto numPacketsToWaitAt = 16384;
				std::vector<u8> packet(maxPacketSize);
				const size_t numPackets = 512 * 1024 * 1024;
				
				auto from = std::chrono::system_clock::now();
				
				for (auto i=0;i<numPackets; ++i)
				{
					auto delay = maxDelay ? rand() % maxDelay : 0;
					if (delay)
						std::this_thread::sleep_for(std::chrono::milliseconds(delay));
					
					size_t packetSize = rand() % maxPacketSize;
					auto *ptr = packet.data();
					auto *end = ptr + packetSize;

					while (ptr != end)
						*ptr++ = nextLocalSend++;
					
					mrudp_send(localConnection, (char *)packet.data(), packetSize, 1);
						
					packetsSent++;
					bytesSent += packetSize;
					
					if (i % numPacketsToWaitAt == 0)
					{
						mrudp_connection_state_t state;
						wait_until(std::chrono::seconds(100), [&]() {
							mrudp_connection_state(localConnection, &state);
							return state.packets_awaiting_ack < 64;
						});
						
						 wait_until(std::chrono::seconds(100), [&]() { return remote.bytesReceived == bytesSent; });

						if (remote.bytesReceived != bytesSent)
						{
							xDebugLine();
						}
						
						auto then = std::chrono::system_clock::now();
						
						std::chrono::duration<double> diff_ = then - from;
						auto diff = diff_.count();
						from = then;
						
						std::cerr << "STAT [" << i << "] " << numPacketsToWaitAt << " in " << diff << " = " << (numPacketsToWaitAt / diff) << std::endl;
					}
				}
				
				THEN("the stream shows up")
				{
					wait_until(std::chrono::seconds(100), [&]() { return remote.bytesReceived == bytesSent; });
					REQUIRE(remote.bytesReceived == bytesSent);
				}
			}
		}
	}
	
}

} // namespace
} // namespace
} // namespace
