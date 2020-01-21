#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <stdx/async/spin_lock.h>

namespace stdx
{
	//屏障
	class _Semaphore
	{
	public:
		//默认构造函数
		_Semaphore()
			:m_lock()
			,m_notify_count(0)
			,m_cv()
		{}
		//析构函数
		~_Semaphore() = default;

		void wait();

		template< class Rep, class Period>
		bool wait_for(const std::chrono::duration<Rep, Period>& rel_time)
		{
			std::unique_lock<stdx::spin_lock> lock(m_lock);
			if (m_cv.wait_for(lock,rel_time,[this](){return m_notify_count != 0;}))
			{
				m_notify_count -= 1;
				return true;
			}
			return false;
		}

		void notify();


	private:
		stdx::spin_lock m_lock;
		int m_notify_count;
		std::condition_variable_any m_cv;
	};
	class semaphore
	{
		using impl_t = std::shared_ptr<stdx::_Semaphore>;
	public:
		semaphore() 
			:m_impl(std::make_shared<_Semaphore>())
		{}
		semaphore(const semaphore &other) 
			:m_impl(other.m_impl)
		{}
		semaphore(semaphore && other) noexcept
			: m_impl(std::move(other.m_impl))
		{}
		~semaphore() = default;
		semaphore &operator=(const semaphore &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		void wait()
		{
			return m_impl->wait();
		}

		void notify()
		{
			return m_impl->notify();
		}

		template< class Rep, class Period>
		bool wait_for(const std::chrono::duration<Rep, Period>& rel_time)
		{
			return m_impl->wait_for(rel_time);
		}

		bool operator==(const semaphore &other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};
}
