#pragma once
//类库遵循以下约定
//所有的Class(除实现Class外,例如:_XxYy)都是引用类型
//所有的Struct(除另外说明外)都是值类型
#include <type_traits>
#ifndef WIN32

#ifndef LINUX

#ifdef __WIN32

#ifndef WIN32
#define WIN32
#endif // !WIN32

#endif // __WIN32

#ifdef __linux__

#ifndef LINUX
#define LINUX
#endif // !LINUX

#endif // __linux__

#endif // !LINUX

#endif // !WIN32

#define int_32 int
#define uint_32 unsigned int
#define int_16 short
#define uint_16 unsigned short

#ifdef WIN32
#define uint unsigned int
#define int_64 __int64
#define uint_64 unsigned __int64
#endif

#ifdef LINUX
#define uint unsigned int
#define int_64 long long int
#define uint_64 unsigned long long int
#endif

#define interface_class class
#define get_byte(x,ptr) *((char*)ptr+(x))
#define delete_copy(type) type(const type &)=delete
#define delete_move(type) type(type&&)=delete
#define cstring const char*
#define empty_cstring ""
#define CRLF "\r\n"

#ifdef WIN32
#define next_line CRLF
#endif

#ifdef LINUX
#define next_line "\n"
#endif

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif // WIN32

#define typename_of(_T) typeid(_T).name()

#define name_of(_Var) #_Var

namespace stdx
{
	union int64_union
	{
		struct 
		{
			uint_32 low;
			int_32 height;
		};
		int_64 value;
	};

	union int32_union
	{
		struct
		{
			uint_16 low;
			int_16 height;
		};
		int_32 value;
	};

	union uint64_union
	{
		struct
		{
			uint_32 low;
			uint_32 height;
		};
		uint_64 value;
	};

	union uint32_union
	{
		struct
		{
			uint_16 low;
			uint_16 height;
		};
		uint_32 value;
	};
}

#include <stdexcept>
#include <system_error>
#include <time.h>
namespace stdx
{
	struct stop_watcher
	{
	public:
		stop_watcher()
			:m_begin(0)
			, m_end(0)
			, m_time(0)
		{}
		~stop_watcher() = default;
		stop_watcher(const stop_watcher &other)
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
		stop_watcher &operator=(const stop_watcher &other)
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
}

namespace stdx
{
	template<uint_64 i>
	struct bin
	{
		enum
		{
			value = ((bin<i / 10>::value) * 2) + (i % 10)
		};
	};

	template<>
	struct bin<0>
	{
		enum
		{
			value = 0
		};
	};
}

namespace stdx
{

	template<typename _T>
	struct _Forwarder
	{
		using forward_type = _T&;
		static forward_type forward(_T &arg)
		{
			return arg;
		}
	};

	template<typename _T>
	struct _Forwarder<_T&&>
	{
		using forward_type = _T&&;
		static forward_type forward(_T &&arg)
		{
			return std::move(arg);
		}
	};

	template<typename _T>
	struct _Forwarder<_T&>
	{
		using forward_type = _T& ;
		static forward_type forward(_T &arg)
		{
			return arg;
		}
	};

	template<typename _T>
	typename _Forwarder<_T>::forward_type forward(typename _Forwarder<_T>::forward_type arg)
	{
		return stdx::_Forwarder<_T>::forward(arg);
	}
}

namespace stdx
{
	template<uint bytes_count>
	struct sys_bit;

	template<>
	struct sys_bit<4>
	{
		using uint_ptr_t = uint_32;
		enum
		{
			bit = 32
		};
	};

	template<>
	struct sys_bit<8>
	{
		using uint_ptr_t = uint_64;
		enum
		{
			bit = 64
		};
	};

	using current_sys_bit = stdx::sys_bit<sizeof(void*)>;
}