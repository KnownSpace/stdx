#pragma once
#include <stdx/traits/base_on.h>
#include <memory>

namespace stdx
{
	class _Any
	{
	public:
		virtual ~_Any() = default;
		virtual void* get() const = 0;
		virtual void set(void* p) = 0;
	};

	template<typename _T>
	class _AnyImpl :public _Any
	{
	public:
		_AnyImpl()
			:m_ptr(nullptr)
		{}

		virtual ~_AnyImpl()
		{
			if (m_ptr)
			{
				delete m_ptr;
			}
		}

		virtual void* get() const override
		{
			return m_ptr;
		}

		virtual void set(void* p) override
		{
			m_ptr = reinterpret_cast<_T*>(p);
		}
	private:
		_T* m_ptr;
	};

	class any
	{
		using self_t = stdx::any;
		using impl_t = std::shared_ptr<stdx::_Any>;
	public:
		any();

		template<typename _Impl,class = typename std::enable_if<stdx::is_base_on<_Impl,stdx::_Any>::value>::type>
		any(const std::shared_ptr<_Impl> &impl)
			:m_impl(impl)
		{}

		any(const self_t& other);

		any(self_t&& other) noexcept;

		self_t& operator=(const self_t& other);

		self_t& operator=(self_t&& other);

		~any() = default;

		void* get() const;

		void set(void* p);

		bool operator==(const self_t& other) const
		{
			return m_impl == other.m_impl;
		}

		operator bool() const
		{
			return (bool)m_impl;
		}
	private:
		impl_t m_impl;
	};

	template<typename _T,typename ..._Args>
	inline stdx::any make_any(_Args &&...args)
	{
		std::shared_ptr<stdx::_AnyImpl<_T>> impl = std::make_shared<stdx::_AnyImpl<_T>>();
		if (!impl)
		{
			throw std::bad_alloc();
		}
		impl->set(new _T(args...));
		return stdx::any(impl);
	}
}