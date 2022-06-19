
#include "../mrudp/mrudp.h"

#include <iostream>
#include "Common.h"


namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("streams")
{
    GIVEN( "mrudp service, remote and local sockets paired" )
    {
		mrudp_addr_t anyAddress;
		mrudp_str_to_addr("127.0.0.1:0", &anyAddress);
		
		State remote("remote");
		remote.service = mrudp_service();
		
		State local("local");
		local.service = mrudp_service();
		
		auto streamSize = 32768;
		std::vector<uint8_t> streamSent;
		std::vector<uint8_t> streamBasis;
		for (int i=0; i<streamSize; ++i)
			streamBasis.push_back((uint8_t)i);
		
		std::vector<uint8_t> streamReceived;
		
		auto remoteConnectionDispatch = Connection {
			[&](auto data, auto size, auto isReliable) {
				auto lock = lock_of(remote.packetsMutex);
				uint8_t nextExpectedData = 0;
				
				if (!streamReceived.empty())
					nextExpectedData = streamReceived.back() + 1;
					
				uint8_t nextData = (uint8_t)data[0];
				if (nextData != nextExpectedData)
				{
					xDebugLine();
				}
				
				streamReceived.insert(streamReceived.end(), data, data+size);
				remote.bytesReceived += size;
				
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
				
			mrudp_listen(remoteSocket, &listen, listenerAccept, listenerClose);
		
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
				auto packetsSent = 0;
				auto bytesSent = 0;
				
				for (auto i=0; i<streamBasis.size(); )
				{
					size_t packetSize = rand() % 2048;
					auto *ptr = &streamBasis[i];
					auto remaining = streamBasis.size() - i;
					
					packetSize = std::min(remaining, packetSize);
					
					streamSent.insert(streamSent.end(), ptr, ptr + packetSize);
					mrudp_send(localConnection, (char *)ptr, packetSize, 1);
					
					i += packetSize;
					packetsSent++;
					bytesSent += packetSize;
				}
				
				THEN("the stream shows up")
				{
					wait_until(std::chrono::seconds(10), [&]() { return remote.bytesReceived == bytesSent; });
				
					REQUIRE(bytesSent == streamBasis.size());
					REQUIRE(streamReceived.size() == streamSent.size());
					
					if (streamReceived != streamSent)
					{
						for (auto i=0; i<streamSize; ++i)
						{
							std::cout << i << ":\t" << (int)streamSent[i] << "\t" << (int)streamReceived[i] << std::endl;
						}

						for (auto i=0; i<streamSize; ++i)
						{
							
							if (streamReceived[i] != streamSent[i])
							{
								xDebugLine();
							}
						}
					}
					
					auto receivedEqualsSent = streamReceived == streamSent;
					REQUIRE(receivedEqualsSent);
				}
			}
		}
	}
	
}

} // namespace
} // namespace
} // namespace
