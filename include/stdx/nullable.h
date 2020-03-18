#pragma once
#include <memory>
#include <stdx/traits/base_on.h>
#include <stdx/traits/construction.h>

namespace stdx
{
	template<typename _T,bool copyable,bool moveable>
	class _Nullable;

	template<typename _T>
	class _Nullable<_T,false,false>
	{
		using ptr_t = std::unique_ptr<_T>;
	public:
		_Nullable()
			:m_ptr(nullptr)
		{};
		virtual ~_Nullable() = default;

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
	protected:
		ptr_t m_ptr;
		_Nullable(_T* ptr)
			:m_ptr(ptr)
		{}
	};

	template<typename _T>
	class _Nullable<_T,true,false>:public virtual stdx::_Nullable<_T,false,false>
	{
		using self_t = stdx::_Nullable<_T, true, false>;
	public:
		_Nullable()
			:stdx::_Nullable<_T,false,false>()
		{}

		virtual ~_Nullable() =default;

		_Nullable(const self_t& other)
			:stdx::_Nullable<_T, false, false>((!other.is_null()) ? (new _T(other.val())) : (nullptr))
		{}

		self_t& operator=(const self_t & other)
		{
			m_ptr.reset((!other.is_null()) ? (new _T(other.val())) : (nullptr));
			return *((self_t*)this);
		}

	private:

	};

	template<typename _T>
	class _Nullable<_T, false, true>:public virtual stdx::_Nullable<_T,false,false>
	{
		using self_t = stdx::_Nullable<_T, false,true>;
	public:
		_Nullable()
			:stdx::_Nullable<_T, false, false>()
		{}

		virtual ~_Nullable() = default;

		_Nullable(self_t && other) noexcept
			:stdx::_Nullable<_T, false, false>((!other.is_null()) ? (new _T(std::move(other.val()))) : (nullptr))
		{
			other.be_null();
		}

		self_t& operator=(self_t &&other) noexcept
		{
			m_ptr.reset((!other.is_null()) ? (new _T(std::move(other.val()))) : (nullptr));
			other.be_null();
			return *((self_t*)this);
		}
	};

	template<typename _T>
	class stdx::_Nullable<_T,true,true>:public stdx::_Nullable<_T,true,false>,public stdx::_Nullable<_T,false,true>
	{
		using self_t = stdx::_Nullable<_T, true, true>;
	public:
		_Nullable()
			:stdx::_Nullable<_T,false,false>()
			,stdx::_Nullable<_T,true,false>()
			,stdx::_Nullable<_T,false,true>()
		{};

		_Nullable(const self_t& other)
			:stdx::_Nullable<_T, false, false>((!other.is_null()) ? (new _T(other.val())) : (nullptr))
			,stdx::_Nullable<_T, false, true>()
			,stdx::_Nullable<_T, true, false>()
		{}

		_Nullable(self_t&& other)
			:stdx::_Nullable<_T, false, false>((!other.is_null()) ? (new _T(std::move(other.val()))) : (nullptr))
			,stdx::_Nullable<_T, true, false>()
			,stdx::_Nullable<_T, false, true>()
		{}

		~_Nullable() = default;

		self_t& operator=(const self_t& other)
		{
			m_ptr.reset((!other.is_null()) ? (new _T(other.val())) : (nullptr));
			return *((self_t*)this);
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_ptr.reset((!other.is_null()) ? (new _T(std::move(other.val()))) : (nullptr));
			other.be_null();
			return *((self_t*)this);
		}
	private:

	};

	template<typename _T>
	using nullable = typename stdx::_Nullable<_T, stdx::have_copy_construction<_T>::value, stdx::have_move_construction<_T>::value>;
}