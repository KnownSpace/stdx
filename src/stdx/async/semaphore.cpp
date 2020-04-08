#include <stdx/async/semaphore.h>

void stdx::_Semaphore::wait()
{
	std::unique_lock<lock_t> lock(m_lock);
	m_cv.wait(lock, [this]() mutable
	{
			return m_notify_count != 0;
	});
	m_notify_count -= 1;
}

void stdx::_Semaphore::notify()
{
	std::unique_lock<lock_t> lock(m_lock);
	m_notify_count += 1;
	m_cv.notify_one();
}