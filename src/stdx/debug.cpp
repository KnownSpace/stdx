#include <stdx/debug.h>

stdx::debug_tracker::debug_tracker()
	:m_counter(0)
	,m_logger(stdx::make_default_logger())
{}

stdx::debug_tracker::debug_tracker(const self_t& other)
	:m_counter(other.m_counter.load())
	,m_logger(other.m_logger)
{}

stdx::debug_tracker::debug_tracker(self_t&& other) noexcept
	:m_counter(std::move(other.m_counter.load()))
	,m_logger(std::move(other.m_logger))
{}

typename stdx::debug_tracker::self_t& stdx::debug_tracker::operator=(const self_t & other)
{
	stdx::debug_tracker tmp(other);
	stdx::atomic_copy(*this,std::move(tmp));
	return *this;
}

typename stdx::debug_tracker::self_t& stdx::debug_tracker::operator=(self_t&& other) noexcept
{
	m_counter = std::move(other.m_counter.load());
	m_logger = std::move(other.m_logger);
	return *this;
}

void stdx::debug_tracker::print_mark()
{
	size_t mark = m_counter.fetch_add(1);
	m_logger.debug(U("[Debug Tracker]Mark: {0}"),mark);
}