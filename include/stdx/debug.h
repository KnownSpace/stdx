#pragma once
#include <stdx/env.h>
#include <stdx/logger.h>
#include <stdx/datetime.h>
#include <atomic>

struct stop_watcher
{
public:
	stop_watcher()
		:m_begin(0)
		, m_end(0)
		, m_time(0)
	{}

	~stop_watcher() = default;

	stop_watcher(const stop_watcher& other)
		:m_begin(other.m_begin)
		, m_end(other.m_end)
		, m_time(other.m_time)
	{}

	void begin()
	{
		m_begin = clock();
	}
	void end()
	{
		m_end = clock();
	}

	clock_t time()
	{
		if (!m_time)
		{
			m_time = m_end - m_begin;
		}
		return m_time;
	}

	stop_watcher& operator=(const stop_watcher& other)
	{
		m_begin = other.m_begin;
		m_end = other.m_end;
		m_time = other.m_time;
		return *this;
	}

	void clean()
	{
		m_begin = 0;
		m_end = 0;
		m_time = 0;
	}
private:
	clock_t m_begin;
	clock_t m_end;
	clock_t m_time;
};

namespace stdx
{
	struct debug_tracker
	{
		using self_t = stdx::debug_tracker;
	public:
		debug_tracker();

		debug_tracker(const self_t &other);

		debug_tracker(self_t&& other) noexcept;

		~debug_tracker() =default;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		void print_mark();

		stdx::logger get_logger() const
		{
			return m_logger;
		}
	private:
		std::atomic_size_t m_counter;
		stdx::logger m_logger;
	};
}