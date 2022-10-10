#pragma once

#include "../Packet.h"
#include "ProxyID.h"

namespace timprepscius {
namespace mrudp {
namespace proxy {

struct Transformer
{
	bool receive_(Packet &packet, ProxyID &id);
	bool receiveForward(Packet &packet, ProxyID &id);
	bool receiveBackward(Packet &packet, ProxyID &id);
	
	bool send_(Packet &packet);
	bool sendBackward(Packet &packet, const ProxyID &id);
	bool sendForward(Packet &packet, const ProxyID &id);
	
	bool sendMagic (Packet &packet);
	bool receiveMagic (Packet &packet);
} ;

} // namespace
} // namespace
} // namespace
