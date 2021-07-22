#include "SendIDGenerator.h"

namespace timprepscius {
namespace mrudp {

SendIDGenerator::SendIDGenerator ()
{
	// there is no need for nextID_ to be particular number
	// and there is no need for the randomness to be securely generated
	// this is only for debugging packet numbers
	nextID_ = ( (rand()%9999) * 1000);
}

PacketID SendIDGenerator::nextID()
{
	return nextID_++;
}

} // namespace
} // namespace
