#pragma once
#include <stdx/env.h>
#include <stdx/string.h>
#include <time.h>

namespace stdx
{
	struct time_span
	{
#ifdef WIN32
		using time_int_t = WORD;
#else
		using time_int_t = int;
#endif
		time_int_t year = 0;
		time_int_t month = 0;
		time_int_t day = 0;
		time_int_t hour = 0;
		time_int_t minute = 0;
		time_int_t second = 0;
		time_int_t millisecond = 0;
	};

	struct datetime
	{
	protected:
#ifdef WIN32
		using time_int_t = WORD;
#else
		using time_int_t = int;
#endif
	public:
		datetime();

		datetime(const datetime& other);

		datetime(datetime &&other) noexcept;

		datetime(const tm &t);

		virtual ~datetime()=default;

		stdx::datetime& operator=(const stdx::datetime& other);

		stdx::datetime& operator=(stdx::datetime&& other) noexcept;

		bool operator==(const stdx::datetime& other) const;

		bool operator>(const stdx::datetime& other) const;

		bool operator<(const stdx::datetime& other) const;

		bool operator>=(const stdx::datetime& other) const
		{
			return !this->operator<(other);
		}

		bool operator<=(const stdx::datetime& other) const
		{
			return !this->operator>(other);
		}

		time_int_t year() const;
		void year(time_int_t v);

		time_int_t month() const;
		void month(time_int_t v);

		time_int_t day() const;
		void day(time_int_t v);

		time_int_t hour() const;
		void hour(time_int_t v);

		time_int_t minute() const;
		void minute(time_int_t v);

		time_int_t second() const;
		void second(time_int_t v);

		time_int_t millisecond() const;
		void millisecond(time_int_t v);

		static time_int_t day_of_month(time_int_t month, time_int_t year);

		void operator+=(const time_span &span);

		void operator-=(const time_span& span);

		datetime operator+(const time_span& span) const
		{
			datetime tmp(*this);
			tmp += span;
			return tmp;
		}

		datetime operator-(const time_span& span) const
		{
			datetime tmp(*this);
			tmp -= span;
			return tmp;
		}

		/*time_span operator-(const stdx::datetime& other) const;*/

		operator tm() const;

		operator time_t() const;

		virtual stdx::string to_string(typename stdx::string::char_t *format) const;

		static stdx::datetime now();

		static stdx::datetime now_utc();

		static time_int_t get_second_by_millisecond(time_int_t v);
	private:
		time_int_t m_year;
		time_int_t m_month;
		time_int_t m_day;
		time_int_t m_hour;
		time_int_t m_minute;
		time_int_t m_second;
		time_int_t m_millisecond;
		void bezero();
	};
}