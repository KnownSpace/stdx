#include <stdx/async/spin_lock.h>

stdx::_SpinLock::_SpinLock()
	:m_locked(false)
{}

void stdx::_SpinLock::lock() volatile
{
	bool exp = false;
	while (m_locked.compare_exchange_strong(exp, true, std::memory_order_acquire))
	{
		exp = false;
		std::this_thread::yield();
	}
}

void stdx::_SpinLock::unlock() volatile noexcept
{
	m_locked.store(false,std::memory_order::memory_order_release);
}