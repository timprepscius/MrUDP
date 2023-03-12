#include "Receiver.h"
#include "../Connection.h"
#include "../Socket.h"
#include "../Service.h"

#include <zlib.h>

namespace timprepscius {
namespace mrudp {

Receiver::Receiver(Connection *connection_) :
	connection(connection_)
{
	receiveQueue.processor =
		[this](auto &packet) {
			processReceived(packet, RELIABLE);
		};

	unreliableReceiveQueue.processor =
		[this](auto &packet) {
			processReceived(packet, UNRELIABLE);
		};
}

void Receiver::open (PacketID packetID)
{
	receiveQueue.processQueue();

	if (status == UNINITIALIZED)
		status = OPEN;
}

void Receiver::close ()
{
	if (status < CLOSED)
	{
		// TODO:
		// this should send even if we haven't finished the handshake
		// as long as we have finished the first packet handshake and have the RSA keys
		// this will help connections time out faster.
		// maybe it should be canSend & also finishedHandshake in the sender enqueue
		
		if (connection->canSend())
		{
			auto packet = strong<Packet>();
			packet->header.type = CLOSE_READ;
		
			connection->sender.sendReliably(packet);
		}
		
		status = CLOSED;
	}
}

void Receiver::fail ()
{
	if (status < CLOSED)
	{
		status = CLOSED;
	}
}

void Receiver::processReceived(ReceiveQueue::Frame &frame, Reliability reliability)
{
	if (frame.header.type == ACK_FRAME)
	{
		Ack ack;
		small_copy((char *)&ack, frame.data, sizeof(ack));
		connection->sender.onAck(ack);
	}
	else
	if (frame.header.type == DATA)
	{
		connection->receive(frame.data, frame.header.dataSize, reliability);
	}
	else
	if (frame.header.type == DATA_COMPRESSED)
	{
		processCompressed(frame);
	}
	else
	if (frame.header.type == CLOSE_WRITE)
	{
		if (reliability == RELIABLE)
		{
			close();
			connection->possiblyClose();
		}
	}
}

void Receiver::processCompressedSubframes(char *begin, int size)
{
	using BufferSize = u32;

	// see SendQueue::coalesceStreamCompressed
	auto end = begin + size;
	
	for (auto p = begin; p < end; )
	{
		FrameTypeID type;
		small_copy((char *)&type, p, sizeof(type));
		p += sizeof(type);
		size -= sizeof(type);
		
		BufferSize frameSize;
		small_copy((char *)&frameSize, p, sizeof(frameSize));
		p += sizeof(frameSize);
		size -= sizeof(frameSize);

		debug_assert(size >= frameSize);
		if (size < frameSize)
			break;
			
		connection->receive(p, frameSize, RELIABLE);
		p += frameSize;
		size -= frameSize;
	}
}

void Receiver::processCompressed(ReceiveQueue::Frame &frame)
{
	using WasCompressed = u8;
	using BufferSize = u32;

	auto &compressed = compressionBuffers[0];
	auto at = compressed.size();
	compressed.resize(compressed.size() + frame.header.dataSize);
	mem_copy(compressed.data() + at, frame.data, frame.header.dataSize);
	
	if (compressed.size() < sizeof(BufferSize))
		return;
		
	char *p = compressed.data();
	auto inSize = compressed.size();
	
	BufferSize bufferSize;
	small_copy((char *)&bufferSize, p, sizeof(BufferSize));
	p += sizeof(BufferSize);
	inSize -= sizeof(BufferSize);
	
	debug_assert(compressed.size() <= bufferSize);
	if (compressed.size() < bufferSize)
		return;
		
	WasCompressed wasCompressed = *p;
	p += sizeof(WasCompressed);
	inSize -= sizeof(WasCompressed);
	
	if (wasCompressed == 0)
	{
		processCompressedSubframes(p, inSize);
	}
	else
	{
		BufferSize uncompressedSize;
		small_copy((char *)&uncompressedSize, p, sizeof(BufferSize));
		p += sizeof(BufferSize);
		inSize -= sizeof(BufferSize);

		auto &uncompressed = compressionBuffers[1];
		debug_assert(uncompressed.empty());
		
		uncompressed.resize(uncompressedSize);
		
		const Bytef *source = (const Bytef *)p;
		uLongf sourceLen = inSize;
		Bytef *dest = (Bytef *)uncompressed.data();
		uLongf destLen = uncompressedSize;
		
		uncompress(dest, &destLen, source, sourceLen);
		debug_assert(destLen == uncompressedSize);

		processCompressedSubframes((char *)dest, (int)destLen);
		uncompressed.resize(0);
	}
	
	compressed.resize(0);
	debug_assert(compressed.empty());
}

inline
bool requiresAck(TypeID typeID)
{
	return
		typeID == DATA_RELIABLE ||
		typeID == PROBE ||
		typeID == CLOSE_READ;
}

void Receiver::onReceive(Packet &packet)
{
	if (status != OPEN)
		return;
		
	// TODO:
	// this needs to keep track of packet ids which arrive out of order, maintain the last good ID
	// and only accept packets after it
		
	auto type = packet.header.type;
	if(requiresAck(type))
	{
		connection->sender.ack(packet.header.id);
	}

	if (packet.header.type == DATA_RELIABLE)
	{
		receiveQueue.onReceive(packet);
	}
	else
	if (packet.header.type == DATA_UNRELIABLE)
	{
		unreliableReceiveQueue.onReceive(packet);
	}
}

} // namespace
} // namespace
