#pragma once
#include <stdx/env.h>
#include <stdx/string.h>
#include <stdio.h>
#include <memory>

namespace stdx
{
	enum class logger_level
	{
		debug,
		info,
		warn,
		error,
		fault
	};

	interface_class basic_logger
	{
		virtual void log(logger_level level,const stdx::string &msg) const = 0;

		interface_class_helper(basic_logger);
	};

	class logger
	{
		using self_t = logger;
		using impl_t = std::shared_ptr<stdx::basic_logger>;
	public:
		explicit logger()
			:m_impl(nullptr)
		{}

		logger(const impl_t& impl)
			:m_impl(impl)
		{}

		logger(const self_t& other)
			:m_impl(other.m_impl)
		{}

		logger(self_t&& other) noexcept
			:m_impl(std::move(other.m_impl))
		{}

		~logger() = default;

		self_t& operator=(const self_t& other)
		{
			m_impl = other.m_impl;
			return *this;
		}

		self_t& operator=(self_t&& other) noexcept
		{
			m_impl = std::move(other.m_impl);
			return *this;
		}

		void log(logger_level level,const stdx::string& msg)
		{
			return m_impl->log(level, msg);
		}
		
		template<typename ..._Args>
		void debug(const stdx::string& format, _Args &&...args)
		{
			stdx::string tmp;
			stdx::format_string(tmp, args...);
			log(stdx::logger_level::debug, tmp);
		}

		template<typename ..._Args>
		void info(const stdx::string& format, _Args &&...args)
		{
			stdx::string tmp(format);
			stdx::format_string(tmp,args...);
			log(stdx::logger_level::info, tmp);
		}

		template<typename ..._Args>
		void warn(const stdx::string& format, _Args&&...args)
		{
			stdx::string tmp(format);
			stdx::format_string(tmp, args...);
			log(stdx::logger_level::warn, tmp);
		}

		template<typename ..._Args>
		void error(const stdx::string& format, _Args&&...args)
		{
			stdx::string tmp(format);
			stdx::format_string(tmp, args...);
			log(stdx::logger_level::error, tmp);
		}

		template<typename ..._Args>
		void fault(const stdx::string& format, _Args&&...args)
		{
			stdx::string tmp(format);
			stdx::format_string(tmp, args...);
			log(stdx::logger_level::fault, tmp);
		}

		template<typename ..._Args>
		void debug(stdx::string&& format, _Args&&...args)
		{
			stdx::format_string(format, args...);
			log(stdx::logger_level::debug, format);
		}

		template<typename ..._Args>
		void info(stdx::string&& format, _Args&&...args)
		{
			stdx::format_string(format, args...);
			log(stdx::logger_level::info, format);
		}

		template<typename ..._Args>
		void warn(stdx::string&& format, _Args&&...args)
		{
			stdx::format_string(format, args...);
			log(stdx::logger_level::warn, format);
		}

		template<typename ..._Args>
		void error(stdx::string&& format, _Args&&...args)
		{
			stdx::format_string(format, args...);
			log(stdx::logger_level::error, format);
		}

		template<typename ..._Args>
		void fault(stdx::string&& format, _Args&&...args)
		{
			stdx::format_string(format, args...);
			log(stdx::logger_level::fault, format);
		}

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

	class _Logger:public stdx::basic_logger
	{
	public:
		_Logger() = default;
		~_Logger() = default;

		virtual void log(logger_level level, const stdx::string& msg) const override;
	private:

	};

	template<typename _Logger,typename ..._Args>
	inline stdx::logger make_logger(_Args &&...args)
	{
		return stdx::logger(std::make_shared<_Logger>(args...));
	}

	extern stdx::logger make_default_logger();

#ifdef WIN32
	class _ANSIColorSupport
	{
	public:
		_ANSIColorSupport();
		~_ANSIColorSupport() = default;

	private:
	};
	extern stdx::_ANSIColorSupport _ansi_color_support;
#endif
}