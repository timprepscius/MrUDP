#include "SendQueue.h"

#include <zlib.h>

namespace timprepscius {
namespace mrudp {

SendQueue::~SendQueue ()
{
	close();
}

void SendQueue::close()
{
	auto lock = lock_of(mutex);
	
	if (status != CLOSED)
	{
		status = CLOSED;
		queue.clear();
	}
}

bool SendQueue::coalescePacket(FrameTypeID type, const u8 *data, size_t size)
{
	if (queue.empty())
		return false;
		
	auto &packet = *queue.back();
	if (packet.dataSize + size + sizeof(FrameHeader) < MAX_PACKET_POST_FRAME_SIZE)
	{
		FrameHeader frameHeader {
			.id = frameIDGenerator.nextID(),
			.type = type,
			.dataSize = FrameHeader::Size(size),
		} ;
		
		pushFrame(packet, frameHeader, data);
		
		return true;
	}
		
	return false;
}

bool SendQueue::coalesceStream(FrameTypeID type, const u8 *data, size_t size)
{
	if (queue.empty())
	{
		auto packet = strong<Packet>();
		queue.push_back(packet);
	}

	auto offset = 0;
	while (size > 0)
	{
		auto &packet = *queue.back();
		auto availableWriteSize = MAX_PACKET_POST_FRAME_SIZE - int(packet.dataSize + sizeof(FrameHeader));
		if (availableWriteSize > 0)
		{
			auto writeSize = std::min((size_t)availableWriteSize, size);
		
			FrameHeader frameHeader {
				.id = frameIDGenerator.nextID(),
				.type = type,
				.dataSize = FrameHeader::Size(writeSize),
			} ;
			
			pushFrame(packet, frameHeader, data + offset);
			
			size -= writeSize;
			offset += writeSize;
		}
		else
		{
			auto packet = strong<Packet>();
			queue.push_back(packet);
		}
	}
		
	return true;
}

bool SendQueue::coalesceStreamCompressed(FrameTypeID type, const u8 *data, size_t size)
{
	using BufferSize = u32;

	auto &compressionBuffer = compressionBuffers[0];
	
	auto at = compressionBuffer.size();
	compressionBuffer.resize(at + sizeof(type) + sizeof(BufferSize) + size);
	
	auto *p = compressionBuffer.data() + at;
	small_copy(p, (char *)&type, sizeof(type));
	p += sizeof(type);
	
	debug_assert(size < std::numeric_limits<BufferSize>().max());
	
	BufferSize size_ = size;
	small_copy(p, (char *)&size_, sizeof(size_));
	p += sizeof(size_);
	
	mem_copy(p, (char *)data, size);
	p += size;
	
	return true;
}

void SendQueue::compress()
{
	using WasCompressed = u8;
	using BufferSize = u32;
	
	auto &uncompressed = compressionBuffers[0];
	auto &compressed = compressionBuffers[1];
	compressed.resize(sizeof(BufferSize) + sizeof(WasCompressed) + sizeof(BufferSize) + uncompressed.size());
	
	auto outSize_ = compressed.data();
	auto wasCompressed_ = outSize_ + sizeof(BufferSize);
	auto uncompressedSize_ = wasCompressed_ + sizeof(WasCompressed);
	auto compressed_ = uncompressedSize_ + sizeof(BufferSize);
	
	const Bytef *source = (const Bytef *)uncompressed.data();
	uLongf sourceLen = uncompressed.size();
	Bytef *dest = (Bytef *)compressed_;
	uLongf destLen = uncompressed.size();
	int level = 9; // options.compression_level;
	
	BufferSize outSize = 0;
	outSize += sizeof(BufferSize);
	outSize += sizeof(WasCompressed);

	if (level > 0 && compress2(dest, &destLen, source, sourceLen, level) == Z_OK)
	{
		BufferSize uncompressedSize__ = (BufferSize)sourceLen;

		*wasCompressed_ = 1;
		core::small_copy((char *)uncompressedSize_, (const char *)&uncompressedSize__, sizeof(BufferSize));
		outSize += sizeof(BufferSize);
		outSize += destLen;

		sLogRelease("mrudp::proxy::compress", logVar(sourceLen) << logVar(destLen));
	}
	else
	{
		sLogRelease("mrudp::proxy::compress", logVar(sourceLen) << "no compression");

		*wasCompressed_ = 0;
		
		auto *uncompressedDest = (wasCompressed_) + sizeof(WasCompressed);
		core::mem_copy((char *)uncompressedDest, (char *)source, sourceLen);
		outSize += sourceLen;
	}
	
	small_copy(outSize_, (char *)&outSize, sizeof(BufferSize));
	
	coalesceStream(DATA_COMPRESSED, (u8*)compressed.data(), outSize);
	compressed.resize(0);
	uncompressed.resize(0);
}

bool SendQueue::coalesce(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode)
{
	if (mode == MRUDP_COALESCE_NONE)
		return false;
		
	if (mode == MRUDP_COALESCE_PACKET)
		return coalescePacket(type, data, size);
		
	if (mode == MRUDP_COALESCE_STREAM)
		return coalesceStream(type, data, size);
		
	if (mode == MRUDP_COALESCE_STREAM_COMPRESSED)
		return coalesceStreamCompressed(type, data, size);
		
	debug_assert(false);
	return false;
}

void SendQueue::enqueue(FrameTypeID type, const u8 *data, size_t size, CoalesceMode mode)
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return;
		
	if (coalesce(type, data, size, mode))
		return;

	auto packet = strong<Packet>();
	FrameHeader frameHeader {
		.id = frameIDGenerator.nextID(),
		.type = type,
		.dataSize = FrameHeader::Size(size),
	} ;
	
	pushFrame(*packet, frameHeader, data);

	queue.push_back(packet);
}

PacketPtr SendQueue::dequeue()
{
	auto lock = lock_of(mutex);
	if (status == CLOSED)
		return nullptr;

	if (queue.empty() && !compressionBuffers[0].empty())
		compress();

	if (queue.empty())
		return nullptr;
		
	auto packet = std::move(queue.front());
	queue.pop_front();
	return packet;
}

bool SendQueue::empty()
{
	auto lock = lock_of(mutex);
	return queue.empty() && compressionBuffers[0].empty();
}

void SendQueue::clear ()
{
	auto lock = lock_of(mutex);
	compressionBuffers[0].resize(0);
	queue.clear();
}

} // namespace
} // namespace
