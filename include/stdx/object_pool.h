#pragma once
#include <stdx/async/spin_lock.h>
#include <stdx/env.h>
#include <list>
#include <stdx/async/thread_local_storer.h>
#include <vector>
namespace stdx
{
	template<typename _T>
	interface_class basic_object_pool
	{
		basic_object_pool(std::function<_T()> &&maker)
			:m_maker(maker)
		{}
		virtual _T get() = 0;
		//move
		virtual void store(_T&& obj) = 0;
		virtual ~basic_object_pool() = default;
		virtual void fill(size_t n) = 0;
		virtual void init()
		{
			//nothing by default
		}
	protected:
		std::function<_T()> m_maker;
	};

	template<typename _T>
	class _DefaultObjectPool:public basic_object_pool<_T>
	{
		using base_t = stdx::basic_object_pool<_T>;
		using self_t = stdx::_DefaultObjectPool<_T>;
	public:
		_DefaultObjectPool(std::function<_T()>&& maker)
			:base_t(std::move(maker))
			,m_lock()
			,m_list()
		{}

		~_DefaultObjectPool()=default;

		virtual _T get() override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			if (m_list.empty())
			{
				lock.unlock();
				return base_t::m_maker();
			}
			else
			{
				_T obj = std::move(m_list.front());
				m_list.pop_front();
				return obj;
			}
		}

		virtual void store(_T&& obj) override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_list.emplace_front(std::move(obj));
		}

		virtual void fill(size_t n) override
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_list.resize(m_list.size()+ n, base_t::m_maker());
		}
	private:
		std::mutex m_lock;
		std::list<_T> m_list;
	};

	template<typename _T>
	class _ConcurrencyObjectPool:public basic_object_pool<_T>
	{
		using base_t = stdx::basic_object_pool<_T>;
		using list_t = std::shared_ptr<std::list<_T>>;
		using vector_t = std::vector<_T>;
		using lock_t = std::shared_ptr<std::mutex>;

		struct cache_list
		{
			vector_t caches;
			list_t list;
			lock_t lock;
			~cache_list()
			{
				if (lock)
				{
					if (list)
					{
						std::unique_lock<std::mutex> _lock(*lock);
						for (auto begin = caches.begin(), end = caches.end(); begin != end; begin++)
						{
							list->emplace_front(std::move(*begin));
						}
					}
				}
			}
		};
		using cache_t = cache_list;
	private:
		cache_t* _GetCache()
		{
			//initzation thread cache
			cache_t* cache = m_cache.get();
			if (!cache)
			{
				m_cache.set(cache_t());
				cache = m_cache.get();
				if (!cache)
				{
					throw std::bad_alloc();
				}
				cache->list = m_list;
				cache->lock = m_lock;
				cache->caches.reserve(m_cache_size);
			}
			return cache;
		}
	public:
		_ConcurrencyObjectPool(std::function<_T()>&& maker)
			:base_t(std::move(maker))
			,m_lock(std::make_shared<std::mutex>())
			,m_list(std::make_shared<std::list<_T>>())
			,m_cache()
			,m_cache_size(32)
		{}

		_ConcurrencyObjectPool(std::function<_T()>&& maker,size_t cache_size)
			:base_t(std::move(maker))
			, m_lock(std::make_shared<std::mutex>())
			, m_list(std::make_shared<std::list<_T>>())
			, m_cache()
			, m_cache_size(cache_size)
		{}

		~_ConcurrencyObjectPool() = default;

		virtual _T get() override
		{
			if (m_cache_size != 0)
			{
				cache_t* cache = _GetCache();
				if (cache->caches.empty())
				{
					if (!m_list->empty())
					{
						//get from gobal pool
						std::unique_lock<std::mutex> lock(*m_lock);
						if (!m_list->empty())
						{
							_T obj = std::move(m_list->front());
							m_list->pop_front();
							return obj;
						}
						else
						{
							lock.unlock();
							return base_t::m_maker();
						}
					}
					else
					{
						return base_t::m_maker();
					}
				}
				//get from thread cache
				_T obj = std::move(cache->caches.front());
				cache->caches.pop_back();
				return obj;
			}
			else
			{
				std::unique_lock<std::mutex> lock(*m_lock);
				if (m_list->empty())
				{
					lock.unlock();
					return base_t::m_maker();
				}
				_T obj = std::move(m_list->front());
				m_list->pop_front();
				return obj;
			}
		}

		virtual void store(_T&& obj) override
		{
			if (m_cache_size != 0)
			{
				cache_t *cache = _GetCache();
				if (cache->caches.size() < m_cache_size)
				{
					cache->caches.emplace_back(std::move(obj));
					return;
				}
			}
			std::unique_lock<std::mutex> lock(*m_lock);
			m_list->emplace_front(std::move(obj));
		}

		virtual void fill(size_t n) override
		{
			std::unique_lock<std::mutex> lock(*m_lock);
			m_list->resize(m_list->size() + n, base_t::m_maker());
		}

		virtual void init()
		{
			m_cache = stdx::make_thread_local_storer<cache_t>();
			if (!m_cache)
			{
				throw std::bad_alloc();
			}
		}
	private:
		//lock for gobal object list
		lock_t m_lock;
		list_t m_list;
		//thread cache
		stdx::thread_local_storer<cache_t> m_cache;
		//cache size
		size_t m_cache_size;
	};

	template<typename _T,template<class> class _Impl>
	class object_pool
	{
		using impl_t = std::shared_ptr<stdx::basic_object_pool<_T>>;
		using self_t = stdx::object_pool<_T, _Impl>;
	public:
		//don't use it.
		//store only.
		explicit object_pool()
			:m_impl(nullptr)
		{}

		object_pool(const std::shared_ptr<_Impl<_T>> &impl)
			:m_impl(impl)
		{}

		object_pool(const object_pool &other)
			:m_impl(other.m_impl)
		{}

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		~object_pool() = default;

		_T get()
		{
			return m_impl->get();
		}

		void store(_T&& obj)
		{
			_T _obj = std::move(obj);
			m_impl->store(std::move(obj));
			return;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		void fill(size_t n)
		{
			return m_impl->fill(n);
		}

		void init()
		{
			return m_impl->init();
		}
	private:
		impl_t m_impl;
	};
	
	template<typename _T>
	using default_object_pool = stdx::object_pool<_T,stdx::_DefaultObjectPool>;

	template<typename _T>
	using concurrency_object_pool = stdx::object_pool<_T, stdx::_ConcurrencyObjectPool>;
	
	template<typename _T,template<class> class _Impl,typename ..._Args>
	inline stdx::object_pool<_T, _Impl> make_object_pool(std::function<_T()> &&maker,_Args&&...args)
	{
		stdx::object_pool<_T, _Impl> pool(std::move(std::make_shared<_Impl<_T>>(std::move(maker),args...)));
		pool.init();
		return pool;
	}

	template<typename _T>
	inline stdx::default_object_pool<_T> make_default_object_pool(std::function<_T()>&& maker)
	{
		return stdx::make_object_pool<_T,stdx::_DefaultObjectPool>(std::move(maker));
	}

	template<typename _T>
	inline stdx::concurrency_object_pool<_T> make_concurrency_object_pool(std::function<_T()>&& maker,size_t cache_size = 32)
	{
		return stdx::make_object_pool<_T, stdx::_ConcurrencyObjectPool>(std::move(maker),cache_size);
	}
}