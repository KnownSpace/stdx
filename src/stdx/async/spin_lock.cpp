#include <stdx/async/spin_lock.h>

stdx::_SpinLock::_SpinLock()
	:m_locked(false)
{}

void stdx::_SpinLock::lock()
{
	while (true)
	{
		bool value = false;
		//将m_locked的值与value比较
		//如果与value相等则返回true,并将true写入m_locked
		//如果不等则返回false,并将m_locked的值写入value
		bool exchanged = m_locked.compare_exchange_strong(value, true);
		if (!value)
		{
			break;
		}
	}
}

void stdx::_SpinLock::unlock() noexcept
{
	m_locked.store(false);
}