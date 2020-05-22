#pragma once
#include <stdx/async/spin_lock.h>
#include <list>
#include <functional>

namespace stdx
{
	class _CallbackFlag
	{
	public:
		using callback_t = std::function<void() noexcept>;

		_CallbackFlag();

		~_CallbackFlag() = default;

		void lock(callback_t &&fn);

		void unlock() noexcept;
	private:
		stdx::spin_lock m_lock;
		bool m_status;
		std::list<callback_t> m_callbacks;
	};

	class callback_flag
	{
		using impl_t = std::shared_ptr<stdx::_CallbackFlag>;
		using self_t = stdx::callback_flag;
		using callback_t = stdx::_CallbackFlag::callback_t;
	public:
		callback_flag()
			:m_impl(std::make_shared<stdx::_CallbackFlag>())
		{}

		callback_flag(const self_t &other)
			:m_impl(other.m_impl)
		{}

		callback_flag(self_t &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~callback_flag() = default;

		self_t &operator=(const self_t &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		void lock(callback_t&& fn)
		{
			return m_impl->lock(std::move(fn));
		}

		void unlock() noexcept
		{
			return m_impl->unlock();
		}

		bool operator==(const self_t& other) const
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