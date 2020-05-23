#include <stdx/poller.h>

std::atomic_size_t stdx::_MutilIndexGenerater(0);

size_t stdx::_GetMutilIndex()
{
	size_t index = stdx::_MutilIndexGenerater.fetch_add(1);
	return index;
}

thread_local size_t stdx::_MutilIndex = stdx::_GetMutilIndex();