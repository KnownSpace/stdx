#include <stdx/buffer.h>
#include <string.h>

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
	if (m_data && (m_size != 0))
	{
		stdx::free(m_data);
	}
}

void stdx::_Buffer::init(const size_t& size)
{
	char* data = (char*)stdx::calloc(size, sizeof(char));
	if (data == nullptr)
	{
		throw std::bad_alloc();
	}
	memset(data, 0, size);
	init(data, size);
}

void stdx::_Buffer::init(char* data, const size_t& size)
{
	if (data != nullptr && size != 0)
	{
		m_data = data;
		m_size = size;
	}
	else
	{
		throw std::invalid_argument("data cannot be nullptr,size cannot be 0");
	}
}

char& stdx::_Buffer::operator[](const size_t& i)
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

char  stdx::_Buffer::operator[](const size_t& i) const
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

void stdx::_Buffer::realloc(const size_t& size)
{
	if (size == 0)
	{
		throw std::invalid_argument("invalid argument: 0");
	}
	if (m_data == nullptr)
	{
		throw std::logic_error("please init first");
	}
	if (size > m_size)
	{
		char* tmp = (char*)stdx::realloc(m_data, m_size);
		if (tmp == nullptr)
		{
			throw std::bad_alloc();
		}
		if (tmp != m_data)
		{
			memcpy(tmp, m_data, m_size);
			m_data = tmp;
		}
		m_size = size;
	}
}

void stdx::_Buffer::set_zero()
{
	if (m_data == nullptr)
	{
		throw std::logic_error("please init first");
	}
	memset(m_data, 0, m_size);
}

void stdx::_Buffer::copy_from(const stdx::_Buffer& other)
{
	auto new_size = other.size();
	if (new_size > m_size)
	{
		realloc(new_size);
	}
	memcpy(m_data, other, new_size);
}

char* stdx::_Buffer::to_raw()
{
	m_size = 0;
	char* buf = m_data;
	m_data = nullptr;
	return buf;
}

stdx::buffer stdx::make_buffer(size_t size)
{
	stdx::buffer buf;
	buf.init(size);
	return buf;
}