#pragma once
#ifndef STDX_PREALLOC_COUNT
#define STDX_PREALLOC_COUNT 20
#endif
#ifndef STDX_CHUNK_SIZE
#define STDX_CHUNK_SIZE 4096
#endif
#include <stdx/async/spin_lock.h>


namespace stdx
{
	struct memory_node
	{
		size_t size;
		memory_node* next;
	};

	struct gobal_memory_pool
	{
	public:
		gobal_memory_pool() noexcept;
		gobal_memory_pool(const stdx::gobal_memory_pool&) = delete;
		gobal_memory_pool(stdx::gobal_memory_pool&&) = delete;
		~gobal_memory_pool() noexcept;
		void* alloc(size_t n) noexcept;
		stdx::memory_node* alloc_node(size_t n) noexcept;
		void dealloc(void* p) noexcept;
	private:
		stdx::spin_lock m_lock;
		stdx::memory_node* m_begin;
		size_t m_size;

		void* get_from_list(size_t n);

		stdx::memory_node* get_node_from_list(size_t n);
	};

	struct thread_memory_pool
	{
	public:
		thread_memory_pool() noexcept;
		thread_memory_pool(const stdx::thread_memory_pool&) = delete;
		thread_memory_pool(stdx::thread_memory_pool&&) = delete;
		~thread_memory_pool() noexcept;
		void* alloc(size_t n) noexcept;
		void dealloc(void *p) noexcept;
	private:
		stdx::memory_node* m_begin;

		void* get_from_list(size_t n);
	};

	extern stdx::gobal_memory_pool _GobalMemoryPool;

	extern thread_local stdx::thread_memory_pool _ThreadMemoryPool;

	extern void* _malloc(size_t n);

	extern void _free(void *p);

	extern void* _calloc(size_t n, size_t m);
}