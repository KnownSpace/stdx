#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <stdx/env.h>
#include <list>
#include <stdx/any.h>
#ifndef WIN32
#include <pthread.h>
#endif

#ifdef WIN32
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						if(_ERROR_CODE != ERROR_IO_PENDING && _ERROR_CODE != NO_ERROR) \
						{ \
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); \
						}
#else
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#endif

namespace stdx
{

	struct memory_list
	{
		using self_t = memory_list;
		using list_t = std::list<stdx::any>;
		using iterator_t = typename list_t::iterator;
		using const_iterator_t = typename list_t::const_iterator;
		using reverse_iterator_t = typename list_t::reverse_iterator;
		using const_reverse_iterator_t = typename list_t::const_reverse_iterator;
	public:
		memory_list();

		memory_list(const self_t& other);

		memory_list(self_t&& other) noexcept;

		~memory_list();

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other) noexcept;

		iterator_t begin();

		const_iterator_t cbegin() const;

		iterator_t end();

		const_iterator_t cend() const;

		reverse_iterator_t rbegin();

		const_reverse_iterator_t crbegin() const;

		reverse_iterator_t rend();

		const_reverse_iterator_t crend() const;

		template<typename _T>
		_T* alloc()
		{
			stdx::any any = stdx::make_any<_T>();
			if (any)
			{
				m_list.push_back(any);
				auto p = reinterpret_cast<_T*>(any.get());
				return p;
			}
			return nullptr;
		}

		size_t size() const;

		void push_back(const stdx::any &mem);

		void pop_back();

		void push_front(const stdx::any& mem);

		void pop_front();

		const stdx::any &front() const;

		const stdx::any &back() const;

		bool empty() const;
	private:
		list_t m_list;
	};

	extern thread_local stdx::memory_list _TLSMemories;

	template<typename _T>
	struct _ThreadLocalStorer
	{
		using self_t = stdx::_ThreadLocalStorer<_T>;
#ifdef WIN32
		using index_t = DWORD;
#else
		using index_t = pthread_key_t;
#endif

	public:
		_ThreadLocalStorer()
#ifdef WIN32
			:m_index(TLS_OUT_OF_INDEXES)
#else
			:m_index(PTHREAD_KEYS_MAX)
#endif
		{}

		~_ThreadLocalStorer()
		{
#ifdef WIN32
			if (m_index != TLS_OUT_OF_INDEXES)
			{
				TlsFree(m_index);
			}
#else
			if (m_index != PTHREAD_KEYS_MAX)
			{
				pthread_key_delete(m_index);
			}
#endif
		}

		_T *get() const
		{
			return _Get();
		}

		void set(const _T& v)
		{
			auto* p = _Get();
			if (p)
			{
				*p = v;
			}
			else
			{
				p = reinterpret_cast<_T*>(_TLSMemories.alloc<_T>());
				if (!p)
				{
					throw std::bad_alloc();
				}
				_Set(p);
			}
		}

		void clean()
		{
			_Set(nullptr);
		}

		bool check() const
		{
			return _Get();
		}

		void init()
		{
			_Init();
		}

	private:
		index_t m_index;

		void _Init()
		{
#ifdef WIN32
			if (m_index == TLS_OUT_OF_INDEXES)
			{
				m_index = TlsAlloc();
				if (m_index == TLS_OUT_OF_INDEXES)
				{
					_ThrowWinError
				}
				_Set(nullptr);
			}
#else
			if (m_index == PTHREAD_KEYS_MAX)
			{
				if (pthread_key_create(&m_index,nullptr))
				{
					_ThrowLinuxError
				}
				_Set(nullptr);
			}
#endif
		}

		_T* _Get() const
		{
			_T* p = nullptr;
#ifdef WIN32
			p = reinterpret_cast<_T*>(TlsGetValue(m_index));
			if (p == nullptr)
			{
				_ThrowWinError
			}
#else
			p = reinterpret_cast<_T*>(pthread_getspecific(m_index));
#endif
			return p;
		}

		void _Set(_T* p)
		{
#ifdef WIN32
			if (TlsSetValue(m_index, reinterpret_cast<LPVOID>(p)) == 0)
			{
				_ThrowWinError
			}
#else
			if (pthread_setspecific(m_index,reinterpret_cast<void*>(p)))
			{
				_ThrowLinuxError
			}
#endif
		}
	};

	template<typename _T>
	class thread_local_storer
	{
		using impl_t = std::shared_ptr<stdx::_ThreadLocalStorer<_T>>;
		using self_t = stdx::thread_local_storer<_T>;
	public:
		thread_local_storer()
			:m_impl(std::make_shared<stdx::_ThreadLocalStorer<_T>>())
		{}

		thread_local_storer(const self_t &other)
			:m_impl(other.m_impl)
		{}

		thread_local_storer(self_t &&other) noexcept
			:m_impl(other.m_impl)
		{}

		~thread_local_storer() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = other.m_impl;
			return *this;
		}

		_T* get() const
		{
			return m_impl->get();
		}
		
		void set(const _T& v)
		{
			return m_impl->set(v);
		}

		void init()
		{
			return m_impl->init();
		}

		void clean()
		{
			return m_impl->clean();
		}

		bool check() const
		{
			return m_impl->check();
		}

		operator bool() const
		{
			return (bool)m_impl;
		}

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}
	private:
		impl_t m_impl;
	};

	template<typename _T>
	inline stdx::thread_local_storer<_T> make_thread_local_storer()
	{
		stdx::thread_local_storer<_T> tls;
		tls.init();
		return tls;
	}
}