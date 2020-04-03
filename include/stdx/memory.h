#pragma once
#ifndef STDX_PREALLOC_COUNT
#define STDX_PREALLOC_COUNT 20
#endif
#ifndef STDX_CHUNK_SIZE
#define STDX_CHUNK_SIZE 4096
#endif


namespace stdx
{
	struct memory_node
	{
		size_t size;
		memory_node* next;
	};

	struct memory_pool
	{
	public:
		memory_pool() noexcept;
		memory_pool(const stdx::memory_pool&) = delete;
		memory_pool(stdx::memory_pool&&) = delete;
		~memory_pool() noexcept;
		void* alloc(size_t n) noexcept;
		void dealloc(void *p) noexcept;
	private:
		memory_node* m_begin;
	};

	extern thread_local memory_pool _MemoryPool;

	extern void* malloc(size_t n);

	extern void free(void *p);
}