#pragma once

#include "../Types.h"

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// SendIDGenerator
//
// SendIDGenerator generates unique packet IDs for a connection.
// --------------------------------------------------------------------------------

template<typename T>
struct IDGenerator
{
	Atomic<T> nextID_;
	
	IDGenerator ();
	T nextID();
};

} // namespace
} // namespace

#include "IDGenerator.inl"
