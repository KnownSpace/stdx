#pragma once
#include <stdx/env.h>
#include <memory>
#include <stdx/string.h>
#include <atomic>

namespace stdx
{
	//自动释放缓存区实现
	class _Buffer
	{
		using ptr_t = char*;
		using data_t = std::atomic<ptr_t>;
	public:
		_Buffer();

		explicit _Buffer(size_t size, char* data);

		~_Buffer();

		void init(const size_t &size);

		void init(char* data, const size_t &size);

		char operator[](const size_t &i) const;

		char &operator[](const size_t &i);

		operator char*()
		{
			return m_data;
		}

		operator const char* () const
		{
			return m_data;
		}

		operator unsigned char* ()
		{
			return (unsigned char*)m_data.load();
		}

		operator const unsigned char* () const
		{
			return (const unsigned char*)m_data.load();
		}

		void realloc(const size_t & size);

		const size_t &size() const
		{
			return m_size;
		}

		void copy_from(const _Buffer &other);

		void set_zero();

		char *move_to_raw();

		void free();

		bool check() const
		{
			return m_data != nullptr;
		}

	private:
		size_t m_size;
		data_t m_data;
	};

	//自动释放缓存区
	class buffer
	{
		using impl_t = std::shared_ptr<stdx::_Buffer>;
	public:
		buffer()
			:m_impl(std::make_shared<_Buffer>())
		{}

		buffer(size_t size, char* data)
			:m_impl(std::make_shared<_Buffer>(size, data))
		{}

		buffer(const buffer &other)
			:m_impl(other.m_impl)
		{}

		buffer(buffer &&other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~buffer() = default;

		void init(size_t size = 4096)
		{
			m_impl->init(size);
		}

		void init(char* data, const size_t &size)
		{
			m_impl->init(data, size);
		}

		operator char*()
		{
			return *m_impl;
		}

		operator const char* () const
		{
			return *m_impl;
		}

		operator unsigned char* ()
		{
			return *m_impl;
		}

		operator const unsigned char* () const
		{
			return *m_impl;
		}

		buffer &operator=(const buffer &other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		char &operator[](const size_t &i)
		{
			return m_impl->operator[](i);
		}

		char operator[](const size_t& i) const
		{
			return m_impl->operator[](i);
		}

		void realloc(size_t size)
		{
			m_impl->realloc(size);
		}

		const size_t &size() const
		{
			return m_impl->size();
		}

		void set_zero()
		{
			return m_impl->set_zero();
		}

		void copy_from(const buffer &other)
		{
			m_impl->copy_from(*other.m_impl);
		}

		bool operator==(const buffer &other)
		{
			return m_impl == other.m_impl;
		}

		char *move_to_raw()
		{
			return m_impl->move_to_raw();
		}

		void free()
		{
			return m_impl->free();
		}

		operator bool() const
		{
			if (m_impl)
			{
				return m_impl->check();
			}
			return false;
		}

		bool check() const
		{
			return m_impl->check();
		}
	private:
		impl_t m_impl;
	};
}

namespace stdx
{
	stdx::buffer make_buffer(size_t size=4096);
}
