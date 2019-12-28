#include <stdx/buffer.h>

stdx::_Buffer::_Buffer()
	:m_size(0)
	, m_data(nullptr)
{}

stdx::_Buffer::_Buffer(size_t size, char* data)
	: m_size(size)
	, m_data(data)
{}

stdx::_Buffer::~_Buffer()
{
	if (m_data)
	{
		free(m_data);
	}
}

void stdx::_Buffer::init(const size_t &size)
{
	char* data = (char*)::calloc(size, sizeof(char));
	if (data == nullptr)
	{
		throw std::bad_alloc();
	}
	::memset(data, 0, size);
	init(data, size);
}

void stdx::_Buffer::init(char * data, const size_t & size)
{
	if (data != nullptr && size != 0)
	{
		m_data = data;
		m_size = size;
	}
}

char &stdx::_Buffer::operator[](const size_t &i) const
{
	if (i >= m_size)
	{
		throw std::out_of_range("out of range");
	}
	if (m_data == nullptr)
	{
		throw std::runtime_error("this buffer is null");
	}
	return *(m_data + i);
}
void stdx::_Buffer::realloc(size_t size)
{
	if (size == 0)
	{
		throw std::invalid_argument("invalid argument: 0");
	}
	if (size > m_size)
	{
		char* tmp = (char*)::realloc(m_data, m_size);
		if (tmp == nullptr)
		{
			throw std::bad_alloc();
		}
		if (tmp != m_data)
		{
			::memcpy(tmp, m_data, m_size);
			m_data = tmp;
		}
		m_size = size;
	}
}

void stdx::_Buffer::copy_from(const stdx::_Buffer &other)
{
	auto new_size = other.size();
	if (new_size > m_size)
	{
		realloc(new_size);
	}
	::memcpy(m_data, other, new_size);
}

char *stdx::_Buffer::to_raw()
{
	m_size = 0;
	char *buf = m_data;
	m_data = nullptr;
	return buf;
}

stdx::buffer stdx::make_buffer(size_t size)
{
	stdx::buffer buf;
	buf.init(size);
	return buf;
}