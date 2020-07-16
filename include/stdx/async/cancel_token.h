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
			,m_after()
		{}

		cancel_token(const self_t& other)
			:m_value(other.m_value)
			,m_after(other.m_after)
		{}

		cancel_token(self_t&& other) noexcept
			:m_value(std::move(other.m_value))
			,m_after(std::move(other.m_after))
		{}

		~cancel_token() = default;

		self_t& operator=(const self_t& other)
		{
			stdx::cancel_token tmp(other);
			stdx::copy_by_move(*this, std::move(tmp));
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_value = std::move(other.m_value);
			m_after = std::move(other.m_after);
			return *this;
		}

		void cancel()
		{
			*m_value = true;
			if (m_after)
			{
				m_after();
			}
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
		
		void swap(stdx::cancel_token& other)
		{
			std::swap(other.m_value, m_value);
		}

		void set_after(std::function<void()>&& after)
		{
			m_after = std::move(after);
		}

		void set_after(std::function<void()>& after)
		{
			m_after = after;
		}
	private:
		std::shared_ptr<value_t> m_value;
		std::function<void()> m_after;
	};
}