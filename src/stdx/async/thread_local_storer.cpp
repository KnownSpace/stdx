#include <stdx/async/thread_local_storer.h>

stdx::memory_list::memory_list()
	:m_list()
{
	m_list.resize(128);
}

stdx::memory_list::memory_list(const self_t& other)
	:m_list(other.m_list)
{}

stdx::memory_list::memory_list(self_t&& other) noexcept
	:m_list(std::move(other.m_list))
{}

stdx::memory_list::~memory_list()
{}

typename stdx::memory_list::self_t& stdx::memory_list::operator=(const self_t& other)
{
	m_list = other.m_list;
	return *this;
}

typename stdx::memory_list::self_t& stdx::memory_list::operator=(self_t&& other) noexcept
{
	m_list = std::move(other.m_list);
	return *this;
}

typename stdx::memory_list::iterator_t stdx::memory_list::begin()
{
	return m_list.begin();
}

typename stdx::memory_list::const_iterator_t stdx::memory_list::cbegin() const
{
	return m_list.cbegin();
}

typename stdx::memory_list::iterator_t stdx::memory_list::end()
{
	return m_list.end();
}

typename stdx::memory_list::const_iterator_t stdx::memory_list::cend() const
{
	return const_iterator_t();
}

typename stdx::memory_list::reverse_iterator_t stdx::memory_list::rbegin()
{
	return reverse_iterator_t();
}

typename stdx::memory_list::const_reverse_iterator_t stdx::memory_list::crbegin() const
{
	return const_reverse_iterator_t();
}

typename stdx::memory_list::reverse_iterator_t stdx::memory_list::rend()
{
	return reverse_iterator_t();
}

typename stdx::memory_list::const_reverse_iterator_t stdx::memory_list::crend() const
{
	return const_reverse_iterator_t();
}

size_t stdx::memory_list::size() const
{
	return m_list.size();
}

void stdx::memory_list::push_back(const stdx::any& mem)
{
	m_list.push_back(mem);
}

void stdx::memory_list::pop_back()
{
	m_list.pop_back();
}

void stdx::memory_list::push_front(const stdx::any& mem)
{
	m_list.push_front(mem);
}

void stdx::memory_list::pop_front()
{
	m_list.pop_front();
}

const stdx::any& stdx::memory_list::front() const
{
	return m_list.front();
}

const stdx::any& stdx::memory_list::back() const
{
	return m_list.back();
}

bool stdx::memory_list::empty() const
{
	return m_list.empty();
}

thread_local stdx::memory_list stdx::_TLSMemories;