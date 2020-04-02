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