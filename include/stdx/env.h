#pragma once
//类库遵循以下约定
//所有的Class(除实现Class外,例如:_XxYy)都是引用类型
//所有的Struct(除另外说明外)都是值类型

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

#ifdef WIN32
#define JEMALLOC_EXPORT
#endif
#include <jemalloc/jemalloc.h>
#include <new>
namespace stdx
{
	extern void *malloc(size_t size);
	extern void free(void* p);
	extern void* calloc(size_t count, size_t size);
	extern int posix_memalign(void **mem,size_t align, size_t size);
	extern void* realloc(void* p, size_t size);
}
#ifdef WIN32
//extern void* operator new(size_t size, const std::nothrow_t& nothrow_value) noexcept;
//extern void operator delete(void* p, const std::nothrow_t& nothrow_value) noexcept;
//extern void* operator new[](size_t size, const std::nothrow_t& nothrow_value) noexcept;
//extern void operator delete[](void* p, const std::nothrow_t& nothrow_value) noexcept;
#endif

#include <type_traits>
#include <stdint.h>
#include <inttypes.h>

#define interface_class struct
#define get_byte(x,ptr) *((char*)ptr+(x))
#define delete_copy(type) type(const type &)=delete
#define delete_move(type) type(type&&)=delete
#define interface_class_helper(type) virtual ~type() = default;\
									type() = default;\
									type(const type&) =default;\
									type(type&&) noexcept = default; \
									type &operator=(const type &) = default;\
									type &operator=(type &&) = default;

#ifdef WIN32
#define CRLF L"\r\n"
#else
#define CRLF "\r\n"
#endif



#ifdef WIN32
#define NEWLINE CRLF
#else
#define NEWLINE "\n"
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

namespace stdx
{
	extern void little_endian_to_big_endian(char *buffer,size_t n);

	extern void big_endian_to_little_endian(char *buffer,size_t n);
}

namespace stdx
{
	template<typename _T,typename = typename std::enable_if<std::is_nothrow_move_assignable<_T>::value>::type>
	void atomic_copy(_T &left,_T &&right) noexcept
	{
		left = std::move(right);
	}
}