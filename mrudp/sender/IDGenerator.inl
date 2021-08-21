#include "IDGenerator.h"

namespace timprepscius {
namespace mrudp {

template<typename T>
IDGenerator<T>::IDGenerator ()
{
	nextID_ = T(0);
}

template<typename T>
T IDGenerator<T>::nextID()
{
	return nextID_++;
}

} // namespace
} // namespace
