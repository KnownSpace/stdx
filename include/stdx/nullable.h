#pragma once
#include <memory>
#include <stdx/traits/base_on.h>
#include <stdx/traits/construction.h>

namespace stdx
{
	template<typename _T>
	class nullable
	{
		using ptr_t = std::unique_ptr<_T>;
		using self_t = stdx::nullable<_T>;
	public:
		nullable()
			:m_ptr(nullptr)
		{}

		nullable(const nullable &other)
			:m_ptr((other.m_ptr)?(new _T(*other.m_ptr)):(nullptr))
		{}

		nullable(nullable&& other) noexcept
			:m_ptr(std::move(other.m_ptr))
		{}

		~nullable() = default;

		operator bool() const
		{
			return m_ptr == nullptr;
		}

		_T& val()
		{
			return *m_ptr;
		}

		const _T& val() const
		{
			return *m_ptr;
		}

		bool is_null() const
		{
			return m_ptr == nullptr;
		}

		void be_null()
		{
			m_ptr.reset(nullptr);
		}

		template<typename ..._Args>
		void set(_Args&&...args)
		{
			m_ptr.reset(new _T(args...));
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		self_t& operator=(const self_t& other)
		{
			m_ptr.reset((!other.is_null()) ? (new _T(other.val())) : (nullptr));
			return *this;
		}
	private:
		ptr_t m_ptr;
	};
}