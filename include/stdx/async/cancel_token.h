#pragma once
#include <memory>
#include <atomic>
#include <stdx/env.h>

namespace stdx
{
	class cancel_token
	{
		using self_t = stdx::cancel_token;
		using value_t = std::atomic_bool;
	public:
		cancel_token()
			:m_value(std::make_shared<value_t>(false))
		{}

		cancel_token(const self_t& other)
			:m_value(other.m_value)
		{}

		cancel_token(self_t&& other) noexcept
			:m_value(std::move(other.m_value))
		{}

		~cancel_token() = default;

		self_t& operator=(const self_t& other)
		{
			m_value = other.m_value;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_value = std::move(other.m_value);
			return *this;
		}

		void cancel()
		{
			*m_value = true;
		}

		bool is_cancel() const
		{
			return *m_value;
		}

		operator bool() const
		{
			return *m_value;
		}

		void reset()
		{
			*m_value = false;
		}

		bool check_ptr() const
		{
			return (bool)m_value;
		}

		bool operator==(const self_t& other)
		{
			return m_value == other.m_value;
		}

	private:
		std::shared_ptr<value_t> m_value;
	};
}