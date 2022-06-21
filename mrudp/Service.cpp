#include "Service.h"
#include "Implementation.h"

namespace timprepscius {
namespace mrudp {

Service::Service (mrudp_imp_selector imp_, void *options)
{
	debug_assert(imp_ == MRUDP_IMP_ASIO);
	
	imp = strong_thread(strong<imp::ServiceImp>(this, (mrudp_options_asio_t *)options));
	
	scheduler = strong<Scheduler>(this);
	scheduler->open();
	
}

void Service::open ()
{
	imp->start();

#ifdef MRUDP_ENABLE_CRYPTO
	crypto = strong<HostCrypto>();
#endif
}

Service::~Service ()
{
	scheduler->close();
	scheduler = nullptr;
	
	imp->stop();
	imp = nullptr;
	
	if (close)
	{
		auto l = lock_of(close->mutex);
		close->value = true;
		close->event.notify_all();
	}
}

// ---------------

ServiceHandle newHandle(const StrongPtr<Service> &service)
{
	return (mrudp_service_t) new StrongPtr<Service>(service);
}

StrongPtr<Service> toNative(ServiceHandle handle_)
{
	if (!handle_)
		return nullptr;

	auto handle = (StrongPtr<Service> *)handle_;
	return (*handle);
}

StrongPtr<Service> closeHandle (ServiceHandle handle_)
{
	if (!handle_)
		return nullptr;
		
	StrongPtr<Service> result;
	auto handle = (StrongPtr<Service> *)handle_;
	std::swap(result, *handle);
	
	return result;
}

void deleteHandle (ServiceHandle handle_)
{
	auto handle = (StrongPtr<Service> *)handle_;
	delete handle;
}

} // namespace
} // namespace
