#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace stdx
{
	//屏障
	class _Semaphore
	{
	public:
		//默认构造函数
		_Semaphore()
			:mutex(std::make_shared<std::mutex>())
			, notify_count(std::make_shared<std::atomic_int>(0))
			, cv(std::make_shared<std::condition_variable>())
		{}
		//析构函数
		~_Semaphore() = default;

		void wait();

		void notify();

		void notify_all();

		template<class _Rep,class _Period>
			bool wait_for(const std::chrono::duration<_Rep, _Period> &time)
		{
			std::unique_lock<std::mutex> lock(*mutex);
			return cv->wait_for(lock, time, [this]() mutable
			{
				int value = notify_count->load();
				if (value == 0)
				{
					return false;
				}
				int new_value = value - 1;
				while (true)
				{
					if (value == 0)
					{
						return false;
					}
					bool exchange = notify_count->compare_exchange_strong(value, new_value);
					if ((!exchange) && (!value))
					{
						return false;
					}
					else if (exchange)
					{
						return true;
					}
				}
			});
		}
	private:
		std::shared_ptr<std::mutex> mutex;
		std::shared_ptr<std::atomic_int> notify_count;
		std::shared_ptr<std::condition_variable> cv;
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
		semaphore(semaphore && other)
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

		void notify_all()
		{
			return m_impl->notify_all();
		}

		template<class _Rep,class _Period>
			bool wait_for(const std::chrono::duration<_Rep, _Period> &time)
		{
			return m_impl->wait_for<_Rep,_Period>(time);
		}

		bool operator==(const semaphore &other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};
}
