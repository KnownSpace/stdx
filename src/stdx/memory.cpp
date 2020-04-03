#include <stdx/memory.h>
#include <stdlib.h>

stdx::memory_pool::memory_pool() noexcept
	:m_begin(nullptr)
{}

stdx::memory_pool::~memory_pool() noexcept
{
	if (m_begin != nullptr)
	{
		memory_node* next = m_begin;
		while (next != nullptr)
		{
			memory_node* _next = next->next;
			::free(next);
			next = _next;
		}
	}
}

void* stdx::memory_pool::alloc(size_t n) noexcept
{
	if (n == 0)
	{
		return nullptr;
	}
	if (n > STDX_CHUNK_SIZE)
	{
		memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + n);
		tmp->size = n;
		tmp->next = nullptr;
		return (char*)tmp + sizeof(memory_node);
	}
	if (m_begin == nullptr)
	{
		memory_node* p = nullptr;
		for (size_t i = 0; i < STDX_PREALLOC_COUNT; i++)
		{
			if (p != nullptr)
			{
				memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node)+ STDX_CHUNK_SIZE);
				if (tmp == nullptr)
				{
					//ÄÚ´æºÄ¾¡
					i = STDX_PREALLOC_COUNT;
					continue;
				}
				tmp->size = STDX_CHUNK_SIZE;
				tmp->next = nullptr;
				p->next = tmp;
				p = p->next;
			}
			else
			{
				p = (memory_node*)::malloc(sizeof(memory_node)+ STDX_CHUNK_SIZE);
				if (p == nullptr)
				{
					//ÄÚ´æºÄ¾¡
					return nullptr;
				}
				p->size = STDX_CHUNK_SIZE;
				p->next = nullptr;
				m_begin = p;
			}
		}
	}
	memory_node *p = m_begin;
	m_begin = p->next;
	p->next = nullptr;
	return (char*)p + sizeof(memory_node);
}

void stdx::memory_pool::dealloc(void* p) noexcept
{
	p = (char*)p - sizeof(memory_node);
	memory_node* new_node = (memory_node*)(p);
	if (new_node->size > STDX_CHUNK_SIZE)
	{
		::free(p);
		return;
	}
	if (m_begin != nullptr)
	{
		memory_node* node = m_begin;
		while (node->next != nullptr)
		{
			node = node->next;
		}
		node->next = new_node;
	}
	else
	{
		m_begin = new_node;
	}
}

thread_local stdx::memory_pool stdx::_MemoryPool;

void* stdx::malloc(size_t n)
{
	return stdx::_MemoryPool.alloc(n);
}

void stdx::free(void* p)
{
	return stdx::_MemoryPool.dealloc(p);
}