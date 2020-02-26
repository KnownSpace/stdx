#pragma once
//类库遵循以下约定
//所有的Class(除实现Class外,例如:_XxYy)都是引用类型
//所有的Struct(除另外说明外)都是值类型
#include <type_traits>
#ifndef WIN32
#ifndef LINUX

//WINDOWS
#if (defined _WINDOWS_) || (defined __WIN32) || (defined _MSC_VER)
#define WIN32
#endif 

//Linux
#if (defined __linux__) || (defined linux) || (defined __gnu_linux__)
#define LINUX
#endif

#endif 
#endif


#include <stdint.h>
#include <inttypes.h>

#define interface_class class
#define get_byte(x,ptr) *((char*)ptr+(x))
#define delete_copy(type) type(const type &)=delete
#define delete_move(type) type(type&&)=delete
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
			uint32_t low;
			int32_t height;
		};
		int64_t value;
	};

	union int32_union
	{
		struct
		{
			uint16_t low;
			int16_t height;
		};
		int32_t value;
	};

	union int16_union
	{
		struct
		{
			uint8_t low;
			int8_t height;
		};
		int16_t value;
	};

	union uint64_union
	{
		struct
		{
			uint32_t low;
			uint32_t height;
		};
		uint64_t value;
	};

	union uint32_union
	{
		struct
		{
			uint16_t low;
			uint16_t height;
		};
		uint32_t value;
	};
	union uint16_union
	{
		struct
		{
			uint8_t low;
			uint8_t height;
		};
		uint16_t value;
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
	template<uint64_t i>
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
	template<uint32_t bytes_count>
	struct sys_bit;

	template<>
	struct sys_bit<4>
	{
		using uint_ptr_t = uint32_t;
		enum
		{
			bit = 32
		};
	};

	template<>
	struct sys_bit<8>
	{
		using uint_ptr_t = uint64_t;
		enum
		{
			bit = 64
		};
	};

	using current_sys_bit = stdx::sys_bit<sizeof(void*)>;
}

namespace stdx
{
	//仅声明但不定义
	template<typename _T>
	typename std::add_lvalue_reference<_T>::type declref();

	template<typename _T>
	typename std::add_lvalue_reference<typename std::add_const<_T>::type>::type declcref();

	template<typename _T>
	typename std::add_rvalue_reference<_T>::type declrref();

	template<typename _T>
	typename std::remove_reference<_T>::type declval();

	template<typename _T>
	typename std::add_pointer<_T>::type declptr();
}