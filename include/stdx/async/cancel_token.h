#pragma once
#include <memory>
#include <atomic>
#include <stdx/env.h>

namespace stdx
{
	class cancel_token
	{
		using self_t = stdx::cancel_token;
	public:
		cancel_token()
			:m_value(std::make_shared<std::atomic_bool>())
		{}

		cancel_token(const self_t& other)
			:m_value(other.m_value)
		{}

		cancel_token(self_t&& other) noexcept
			:m_value(other.m_value)
		{}

		~cancel_token() = default;

		self_t& operator=(const self_t& other)
		{
			m_value = other.m_value;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_value = other.m_value;
			return *this;
		}

		void cancel()
		{
			m_value->store(true);
		}

		bool is_cancel() const
		{
			return m_value->load();
		}

		operator bool() const
		{
			return m_value->load();
		}

		void reset()
		{
			m_value->store(false);
		}

	private:
		std::shared_ptr<std::atomic_bool> m_value;
	};
}