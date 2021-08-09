#pragma once

#include "Base.h"
#include "mrudp.h"
#include "Crypto.h"

namespace timprepscius {
namespace mrudp {

namespace imp {
struct ServiceImp;
}

// --------------------------------------------------------------------------------
// Service
//
// Service serves to house the Clock, the Random number generator, and ServiceImp.
//
// The ServiceImp is generally responsible for scheduling events.
// --------------------------------------------------------------------------------

struct Service : StrongThis<Service>
{
	Service (mrudp_imp_selector imp, void *options);
	~Service ();
	
	void open ();
	
	struct Close
	{
		Mutex mutex;
		Event event;
		bool value = false;
	} ;
	
	// If the close field is set on destruction, the service
	// will lock the mutex, set the value and event and unlock the mutex.
	Close *close = nullptr;
	
	Clock clock;
	Random random;
	StrongPtr<imp::ServiceImp> imp;	

#ifdef MRUDP_ENABLE_CRYPTO
	StrongPtr<HostCrypto> crypto;
#endif
} ;

// --------------------------------------------------------
// handles
// --------------------------------------------------------

typedef mrudp_service_t ServiceHandle;

ServiceHandle newHandle(const StrongPtr<Service> &connection);
StrongPtr<Service> toNative(ServiceHandle handle);
StrongPtr<Service> closeHandle(ServiceHandle handle);
void deleteHandle (ServiceHandle handle);


} // namespace
} // namespace
