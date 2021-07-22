#include "Service.h"
#include "Implementation.h"

namespace timprepscius {
namespace mrudp {

Service::Service ()
{
}

void Service::open ()
{
	imp = strong_thread(strong<imp::ServiceImp>(this));
	imp->start();
}

Service::~Service ()
{
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
