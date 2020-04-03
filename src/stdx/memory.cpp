#include <stdx/memory.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#ifdef DEBUG
#include <stdio.h>
#endif

stdx::gobal_memory_pool::gobal_memory_pool() noexcept
	:m_lock()
	,m_begin(nullptr)
	,m_size(0)
{}

stdx::gobal_memory_pool::~gobal_memory_pool() noexcept
{
#ifdef DEBUG
	size_t count = 0;
#endif
	if (m_begin != nullptr)
	{
		memory_node* next = m_begin;
		while (next != nullptr)
		{
			memory_node* _next = next->next;
			::free(next);
			next = _next;
#ifdef DEBUG
			count += 1;
#endif
		}
#ifdef DEBUG
		::printf("[Memory Pool]成功释放%zu个Chunk\n", count);
#endif
	}
}

stdx::memory_node* stdx::gobal_memory_pool::get_node_from_list(size_t n)
{
	stdx::memory_node* p = m_begin, * pre = p;
	while (p != nullptr)
	{
		if (p->size >= n)
		{
			if (p == m_begin)
			{
				m_begin = m_begin->next;
				p->next = nullptr;
				return p;
			}
			else
			{
				pre->next = p->next;
				p->next = nullptr;
				return p;
			}
		}
		pre = p;
		p = p->next;
	}
	return nullptr;
}

void* stdx::gobal_memory_pool::get_from_list(size_t n)
{
	stdx::memory_node* p = get_node_from_list(n);
	if (p == nullptr)
	{
		return nullptr;
	}
	void* tmp = (char*)p + sizeof(stdx::memory_node);
	return tmp;
}

stdx::memory_node* stdx::gobal_memory_pool::alloc_node(size_t n) noexcept
{
	if (n > STDX_CHUNK_SIZE)
	{
		void* tmp = ::malloc(sizeof(stdx::memory_node) + n);
		if (tmp == nullptr)
		{
			return nullptr;
		}
		stdx::memory_node* node = (stdx::memory_node*)tmp;
		node->next = nullptr;
		node->size = n;
		return node;
	}
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	stdx::memory_node* p = get_node_from_list(n);
	if (p != nullptr)
	{
		return p;
	}
	lock.unlock();
	p = (stdx::memory_node*)::malloc(sizeof(stdx::memory_node) + n);
	if (p == nullptr)
	{
		return nullptr;
	}
	p->size = n;
	p->next = nullptr;
	return p;
}

void* stdx::gobal_memory_pool::alloc(size_t n) noexcept
{
	stdx::memory_node* node = alloc_node(n);
	void *p = (char*)node + sizeof(stdx::memory_node);
	return p;
}

void stdx::gobal_memory_pool::dealloc(void* p) noexcept
{
	if (p == nullptr)
	{
		return;
	}
	p = (char*)p - sizeof(stdx::memory_node);
	stdx::memory_node* new_node = (stdx::memory_node*)p;
	if (new_node->size > STDX_CHUNK_SIZE)
	{
		::free(new_node);
		return;
	}
	std::unique_lock<stdx::spin_lock> lock(m_lock);
	new_node->next = m_begin;
	m_begin = new_node;
}


stdx::thread_memory_pool::thread_memory_pool() noexcept
	:m_begin(nullptr)
{}


void* stdx::thread_memory_pool::get_from_list(size_t n)
{
	stdx::memory_node* p = m_begin, * pre = p;
	while (p != nullptr)
	{
		if (p->size >= n)
		{
			if (p == m_begin)
			{
				m_begin = m_begin->next;
				p->next = nullptr;
				void* tmp = (char*)p + sizeof(stdx::memory_node);
				return tmp;
			}
			else
			{
				pre->next = p->next;
				p->next = nullptr;
				void* tmp = (char*)p + sizeof(stdx::memory_node);
				return tmp;
			}
		}
		pre = p;
		p = p->next;
	}
	return nullptr;
}

stdx::thread_memory_pool::~thread_memory_pool() noexcept
{
	if (m_begin != nullptr)
	{
		memory_node* next = m_begin;
		while (next != nullptr)
		{
			memory_node* _next = next->next;
			stdx::_GobalMemoryPool.dealloc(_next);
			next = _next;
		}
	}
}

void* stdx::thread_memory_pool::alloc(size_t n) noexcept
{
	if (n > STDX_CHUNK_SIZE)
	{
		void* tmp = ::malloc(sizeof(stdx::memory_node) + n);
		if (tmp == nullptr)
		{
			return nullptr;
		}
		stdx::memory_node* node = (stdx::memory_node*)tmp;
		node->next = nullptr;
		node->size = n;
		tmp = (char*)tmp + sizeof(stdx::memory_node);
		return tmp;
	}
	void* p = get_from_list(n);
	if (p != nullptr)
	{
		return p;
	}
	for (size_t i = 0; i < STDX_PREALLOC_COUNT; i++)
	{
		stdx::memory_node *node = stdx::_GobalMemoryPool.alloc_node(n);
		if (node != nullptr)
		{
			node->next = m_begin;
			m_begin = node;
		}
		else
		{
			break;
		}
	}
	if (m_begin == nullptr)
	{
		return nullptr;
	}
	else
	{
		auto *node = get_from_list(n);
		if(node == nullptr)
		{
			return nullptr;
		}
		return (char*)node + sizeof(stdx::memory_node);
	}
}

void stdx::thread_memory_pool::dealloc(void* p) noexcept
{
	if(m_begin == nullptr)
	{
		if(p != nullptr)
		{
			p = (char*)p-sizeof(stdx::memory_node);
			stdx::memory_node *node = (stdx::memory_node*)p;
			if(node->size > STDX_CHUNK_SIZE)
			{
				::free(node);
				return;
			}
			else
			{
				m_begin = node;
			}
		}
	}
	else
	{
		stdx::_GobalMemoryPool.dealloc(p);
	}
}

//void* stdx::memory_pool::alloc(size_t n) noexcept
//{
//	if (n == 0)
//	{
//		return nullptr;
//	}
//
//	if (n > STDX_CHUNK_SIZE)
//	{
//		memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + n);
//		tmp->size = n;
//		tmp->next = nullptr;
//		return (char*)tmp + sizeof(memory_node);
//	}
//
//	if (m_begin == nullptr)
//	{
//		if (!cache(n))
//		{
//			return nullptr;
//		}
//		memory_node *p = m_begin;
//		m_begin = p->next;
//		p->next = nullptr;
//		return (char*)p + sizeof(memory_node);
//	}
//
//	if (m_begin->size >= n)
//	{
//		memory_node* p = m_begin;
//		m_begin = p->next;
//		p->next = nullptr;
//		return (char*)p + sizeof(memory_node);
//	}
//	else
//	{
//		//寻找内存
//		memory_node* p = m_begin, *pre = nullptr;
//		while (p->size < n)
//		{
//			pre = p;
//			p = p->next;
//			if (p == nullptr)
//			{
//				break;
//			}
//		}
//
//		//找到了
//		if (p != nullptr)
//		{
//			pre->next = p->next;
//			p->next = nullptr;
//			return (char*)p + sizeof(memory_node);
//		}
//		else
//		{
//			if (!cache(n))
//			{
//				return nullptr;
//			}
//			p = m_begin;
//			m_begin = p->next;
//			p->next = nullptr;
//			return (char*)p + sizeof(memory_node);
//		}
//	}
//}
//
//void stdx::memory_pool::dealloc(void* p) noexcept
//{
//	if (p == nullptr)
//	{
//		return;
//	}
//	p = (char*)p - sizeof(memory_node);
//	memory_node* new_node = (memory_node*)p;
//	if (new_node->size > STDX_CHUNK_SIZE)
//	{
//		::free(p);
//		return;
//	}
//	new_node->next = m_begin;
//	m_begin = new_node;
//}
//
//bool stdx::memory_pool::cache(size_t n)
//{
////#ifdef DEBUG
//	printf("[Memory Pool]开始缓存内存\n");
////#endif
//	for (size_t i = 0; i < STDX_PREALLOC_COUNT; i++)
//	{
//		if (m_begin != nullptr)
//		{
//			memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + STDX_CHUNK_SIZE);
//			if (tmp == nullptr)
//			{
//				//内存耗尽
//				i = STDX_PREALLOC_COUNT;
//				continue;
//			}
//			tmp->size = STDX_CHUNK_SIZE;
//			tmp->next = m_begin;
//			m_begin = tmp;
//		}
//		else
//		{
//			memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + STDX_CHUNK_SIZE);
//			if (tmp == nullptr)
//			{
//				//内存耗尽
//				return false;
//			}
//			tmp->size = STDX_CHUNK_SIZE;
//			tmp->next = nullptr;
//			if (m_begin == nullptr)
//			{
//				m_begin = tmp;
//			}
//			else
//			{
//				tmp->next = m_begin;
//				m_begin = tmp;
//			}
//		}
//	}
//	return true;
//}

//void* stdx::thread_memory_pool::alloc(size_t n) noexcept
//{
//	if (n == 0)
//	{
//		return nullptr;
//	}
//	if (n > STDX_CHUNK_SIZE)
//	{
//		memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + n);
//		if (tmp == nullptr)
//		{
//			return nullptr;
//		}
//		tmp->size = n;
//		tmp->next = nullptr;
//		return (char*)tmp + sizeof(memory_node);
//	}
//	//std::lock_guard<stdx::spin_lock> lock(m_lock);
//	if (m_begin == nullptr)
//	{
//		m_count += 1;
//		if (m_free != 0)
//		{
//			return nullptr;
//		}
//#ifdef DEBUG 
//		printf("[Memory Pool]线程:%d正在缓存内存,已缓存%zu次\n", std::this_thread::get_id(), m_count);
//#endif
//		//{
//		//	size_t i = 0;
//		//	bool token = true;
//		//	while (token)
//		//	{
//		//		if (i >= STDX_PREALLOC_COUNT)
//		//		{
//		//			token = false;
//		//			continue;
//		//		}
//		//		m_free += 1;
//		//	}
//		//}
//		for (size_t i = 0; i < STDX_PREALLOC_COUNT; i++)
//		{
//			if (m_begin != nullptr)
//			{
//				memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + STDX_CHUNK_SIZE);
//				if (tmp == nullptr)
//				{
//					i = STDX_PREALLOC_COUNT;
//					continue;
//				}
//				tmp->size = STDX_CHUNK_SIZE;
//				tmp->next = m_begin;
//				m_begin = tmp;
//			}
//			else
//			{
//				memory_node* tmp = (memory_node*)::malloc(sizeof(memory_node) + STDX_CHUNK_SIZE);
//				if (tmp == nullptr)
//				{
//					return nullptr;
//				}
//				tmp->size = STDX_CHUNK_SIZE;
//				tmp->next = m_begin;
//				m_begin = tmp;
//			}
//			m_free += 1;
//		}
//		m_free -= 1;
//		memory_node* p = m_begin;
//		m_begin = m_begin->next;
//		p->next = nullptr;
//		return (char*)p + sizeof(memory_node);
//	}
//	else
//	{
//#ifdef DEBUG
//		printf("[Memory Pool]线程:%d使用回收内存,剩余回收内存%zu\n", std::this_thread::get_id(), m_free);
//#endif
//		m_free -= 1;
//		memory_node* p = m_begin;
//		m_begin = m_begin->next;
//		p->next = nullptr;
//		return (char*)p + sizeof(memory_node);
//	}
//}
//
//void stdx::thread_memory_pool::dealloc(void* p) noexcept
//{
//	if (p == nullptr)
//	{
//		return;
//	}
//	p = (char*)p - sizeof(memory_node);
//	memory_node* new_node = (memory_node*)(p);
//	//std::lock_guard<stdx::spin_lock> lock(m_lock);
//	if (new_node->size > STDX_CHUNK_SIZE /*|| m_free > STDX_PREALLOC_COUNT*/)
//	{
//		::free(p);
//		return;
//	}
//	m_free += 1;
//	new_node->next = m_begin;
//	m_begin = new_node;
//#ifdef DEBUG
//	printf("[Memory Pool]归还内存%zu bytes\n", new_node->size);
//#endif
//}

stdx::gobal_memory_pool stdx::_GobalMemoryPool;

thread_local stdx::thread_memory_pool stdx::_ThreadMemoryPool;

void* stdx::_malloc(size_t n)
{
	return stdx::_ThreadMemoryPool.alloc(n);
}

void stdx::_free(void* p)
{
	return stdx::_ThreadMemoryPool.dealloc(p);
}

void* stdx::_calloc(size_t n, size_t m)
{
	return stdx::_malloc(n*m);
}
