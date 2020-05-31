#include <stdx/poller.h>

std::atomic_size_t stdx::_MultiIndexGenerater(0);

size_t stdx::_GetMultiIndex()
{
	size_t index = stdx::_MultiIndexGenerater.fetch_add(1);
	return index;
}

thread_local size_t stdx::_MultiIndex = stdx::_GetMultiIndex();