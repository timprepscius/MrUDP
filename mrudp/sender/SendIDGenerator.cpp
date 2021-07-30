#include "SendIDGenerator.h"

namespace timprepscius {
namespace mrudp {

SendIDGenerator::SendIDGenerator ()
{
	nextID_ = 0;
}

PacketID SendIDGenerator::nextID()
{
	return nextID_++;
}

} // namespace
} // namespace
