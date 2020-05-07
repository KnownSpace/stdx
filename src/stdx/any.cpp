#include <stdx/any.h>

stdx::any::any()
	:m_impl(nullptr)
{
}

stdx::any::any(const self_t& other)
	:m_impl(other.m_impl)
{}

stdx::any::any(self_t&& other) noexcept
	:m_impl(std::move(other.m_impl))
{}

typename stdx::any::self_t& stdx::any::operator=(const self_t& other)
{
	m_impl = other.m_impl;
	return *this;
}

typename stdx::any::self_t& stdx::any::operator=(self_t&& other) noexcept
{
	m_impl = std::move(other.m_impl);
	return *this;
}

void* stdx::any::get() const
{
	return m_impl->get();
}

void stdx::any::set(void* p)
{
	m_impl->set(p);
}