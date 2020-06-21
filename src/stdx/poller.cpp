#include <stdx/poller.h>

std::atomic_size_t stdx::_MultipollerIdGenerater(0);

thread_local size_t stdx::_MultipollerId = 0;

thread_local bool stdx::_HasMultipollerId = false;