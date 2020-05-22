#include <stdx/async/callback_flag.h>
#include <mutex>

stdx::_CallbackFlag::_CallbackFlag()
	:m_lock()
	,m_status(false)
	,m_callbacks()
{}

void stdx::_CallbackFlag::lock(callback_t && fn)
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (!m_status)
	{
		m_status = true;
		lock.unlock();
		fn();
		return;
	}
	m_callbacks.push_back(std::move(fn));
}

void stdx::_CallbackFlag::unlock() noexcept
{
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	if (m_callbacks.empty())
	{
		m_status = false;
		return;
	}
	auto fn = m_callbacks.front();
	m_callbacks.pop_front();
	lock.unlock();
	fn();
}