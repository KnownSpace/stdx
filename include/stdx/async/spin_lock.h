#pragma once
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>

namespace stdx
{
	class _SpinLock
	{
	public:
		_SpinLock();
		~_SpinLock() = default;

		void lock();

		void unlock() noexcept;
	private:
		volatile std::atomic<bool> m_locked;
	};
	class spin_lock
	{
		using impl_t = std::shared_ptr<stdx::_SpinLock>;
	public:
		spin_lock()
			:m_impl(std::make_shared<stdx::_SpinLock>())
		{}
		spin_lock(const spin_lock &other)
			:m_impl(other.m_impl)
		{}
		spin_lock(spin_lock &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}
		~spin_lock() = default;
		spin_lock &operator=(const spin_lock &other)
		{
			m_impl = other.m_impl;
			return *this;
		}
		void lock()
		{
			m_impl->lock();
		}
		void unlock() noexcept
		{
			m_impl->unlock();
		}

		bool operator==(const spin_lock &other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

	private:
		impl_t m_impl;
	};
}
