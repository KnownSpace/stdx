#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace stdx
{
	//����
	class _Barrier
	{
	public:
		//Ĭ�Ϲ��캯��
		_Barrier()
			:mutex(std::make_shared<std::mutex>())
			, notify_count(0)
			, cv(std::make_shared<std::condition_variable>())
		{}
		//��������
		~_Barrier()
		{
		}

		//�ȴ�ͨ��
		void wait()
		{
			std::unique_lock<std::mutex> lock(*mutex);
			auto &n = notify_count;
			cv->wait(lock, [&n]() { return (int)n; });
			notify_count -= 1;
		}
		//ͨ��
		void pass()
		{
			cv->notify_one();
			notify_count += 1;
		}
		template<class _Rep,class _Period>
			bool wait_for(const std::chrono::duration<_Rep, _Period> &time)
		{
			std::unique_lock<std::mutex> lock(*mutex);
			auto &n = notify_count;
			cv->wait_for(lock, time, [&n]() { return (int)n; });
			notify_count -= 1;
		}
	private:
		std::shared_ptr<std::mutex> mutex;
		std::atomic_int notify_count;
		std::shared_ptr<std::condition_variable> cv;
	};
	class barrier
	{
		using impl_t = std::shared_ptr<_Barrier>;
	public:
		barrier()
			:m_impl(std::make_shared<_Barrier>())
		{}
		barrier(const barrier &other)
			:m_impl(other.m_impl)
		{}
		barrier(barrier &&other)
			:m_impl(std::move(other.m_impl))
		{}
		~barrier() = default;
		barrier &operator=(const barrier &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		void wait()
		{
			return m_impl->wait();
		}
		void pass()
		{
			return m_impl->pass();
		}
		template<class _Rep,class _Period>
			bool wait_for(const std::chrono::duration<_Rep, _Period> &time)
		{
			return m_impl->wait_for<_Rep,_Period>(time);
		}
	private:
		impl_t m_impl;
	};
}