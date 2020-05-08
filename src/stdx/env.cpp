#include <stdx/env.h>

void stdx::little_endian_to_big_endian(char* buffer, size_t n)
{
	if (n==0)
	{
		return;
	}
	size_t begin(0),end(n-1);
	while (begin<end)
	{
		char tmp(buffer[end]);
		buffer[end] = buffer[begin];
		buffer[begin] = tmp;
		begin++;
		end--;
	}
}

void stdx::big_endian_to_little_endian(char* buffer, size_t n)
{
	little_endian_to_big_endian(buffer, n);
}

void* stdx::malloc(size_t size)
{
#ifdef WIN32
	return je_malloc(size);
#else
	return ::malloc(size);
#endif
}

void stdx::free(void* p)
{
#ifdef WIN32
	return je_free(p);
#else
	return ::free(p);
#endif
}

void* stdx::calloc(size_t count, size_t size)
{
#ifdef WIN32
	return je_calloc(count, size);
#else
	return ::calloc(count, size);
#endif
}

int stdx::posix_memalign(void** mem, size_t align, size_t size)
{
#ifdef WIN32
	return je_posix_memalign(mem, align, size);
#else
	return ::posix_memalign(mem, align, size);
#endif
}

void* stdx::realloc(void* p, size_t size)
{
#ifdef WIN32
	return je_realloc(p, size);
#else
	return ::realloc(p, size);
#endif
}

#ifdef WIN32
//void* operator new(size_t size, const std::nothrow_t& nothrow_value) noexcept
//{
//	return stdx::malloc(size);
//}
//
//void operator delete(void* p, const std::nothrow_t& nothrow_value) noexcept
//{
//	return stdx::free(p);
//}
//
//void* operator new[](size_t size, const std::nothrow_t& nothrow_value) noexcept
//{
//	return stdx::malloc(size);
//}
//
//void operator delete[](void* p, const std::nothrow_t& nothrow_value) noexcept
//{
//	return stdx::free(p);
//}
#endif
