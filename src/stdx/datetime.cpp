#include <stdx/datetime.h>

stdx::datetime::datetime()
	:m_year(1970)
	, m_month(1)
	, m_day(1)
	, m_hour(0)
	, m_minute(0)
	, m_second(0)
	, m_millisecond(0)
{}

stdx::datetime::datetime(const datetime& other)
	:m_year(other.m_year)
	,m_month(other.m_month)
	,m_day(other.m_day)
	,m_hour(other.m_hour)
	,m_minute(other.m_minute)
	,m_second(other.m_second)
	,m_millisecond(other.m_millisecond)
{}

stdx::datetime::datetime(datetime&& other) noexcept
	:m_year(std::move(other.m_year))
	, m_month(std::move(other.m_month))
	, m_day(std::move(other.m_day))
	, m_hour(std::move(other.m_hour))
	, m_minute(std::move(other.m_minute))
	, m_second(std::move(other.m_second))
	, m_millisecond(std::move(other.m_millisecond))
{}

stdx::datetime& stdx::datetime::operator=(const stdx::datetime& other)
{
	m_year = other.m_year;
	m_month = other.m_month;
	m_day = other.m_day;
	m_hour = other.m_hour;
	m_minute = other.m_minute;
	m_second = other.m_second;
	m_millisecond = other.m_millisecond;
	return *this;
}

stdx::datetime& stdx::datetime::operator=(stdx::datetime &&other) noexcept
{
	m_year = other.m_year;
	m_month = other.m_month;
	m_day = other.m_day;
	m_hour = other.m_hour;
	m_minute = other.m_minute;
	m_second = other.m_second;
	m_millisecond = other.m_millisecond;
	return *this;
}

bool stdx::datetime::operator==(const stdx::datetime& other) const
{
	return (m_year == other.m_year && m_year == other.m_month && m_day == other.m_day && m_hour == other.m_hour && m_minute == other.m_minute && m_second == other.m_second && m_millisecond == other.m_millisecond);
}

bool stdx::datetime::operator>(const stdx::datetime& other) const
{
	if (m_year > other.m_year)
	{
		return true;
	}
	else if (m_year < other.m_year)
	{
		return false;
	}

	if (m_month > other.m_month)
	{
		return true;
	}
	else if (m_month < other.m_month)
	{
		return false;
	}

	if (m_day > other.m_day)
	{
		return true;
	}
	else if (m_day < other.m_day)
	{
		return false;
	}

	if (m_hour > other.m_hour)
	{
		return true;
	}
	else if (m_hour < other.m_hour)
	{
		return false;
	}

	if (m_minute > other.m_minute)
	{
		return true;
	}
	else if(m_minute < other.m_minute)
	{
		return false;
	}

	if (m_second > other.m_second)
	{
		return true;
	}
	else if (m_second < other.m_second)
	{
		return false;
	}

	return (m_millisecond > other.m_millisecond);
}

bool stdx::datetime::operator<(const stdx::datetime& other) const
{
	if (m_year < other.m_year)
	{
		return true;
	}
	else if (m_year > other.m_year)
	{
		return false;
	}

	if (m_month < other.m_month)
	{
		return true;
	}
	else if (m_month > other.m_month)
	{
		return false;
	}

	if (m_day < other.m_day)
	{
		return true;
	}
	else if (m_day > other.m_day)
	{
		return false;
	}

	if (m_hour < other.m_hour)
	{
		return true;
	}
	else if (m_hour > other.m_hour)
	{
		return false;
	}

	if (m_minute < other.m_minute)
	{
		return true;
	}
	else if (m_minute > other.m_minute)
	{
		return false;
	}

	if (m_second < other.m_second)
	{
		return true;
	}
	else if (m_second > other.m_second)
	{
		return false;
	}

	return (m_millisecond < other.m_millisecond);
}

typename stdx::datetime::time_int_t stdx::datetime::year() const
{
	return m_year;
}

void stdx::datetime::year(time_int_t v)
{
	if (v<1970)
	{
		throw std::invalid_argument("year can not less than 1970");
	}
	m_year = v;
}

typename stdx::datetime::time_int_t stdx::datetime::month() const
{
	return m_month;
}

void stdx::datetime::month(time_int_t v)
{
	if (v<1 || v>12)
	{
		throw std::invalid_argument("month can not less than 1 or larger than 12");
	}
	m_month = v;
}

typename stdx::datetime::time_int_t stdx::datetime::day() const
{
	return m_day;
}

void stdx::datetime::day(time_int_t v)
{
	if (v < 0)
	{
		throw std::invalid_argument("day cannot less than 0");
	}
	if (m_month == 2)
	{
		if (m_year % 4)
		{
			if (v>28)
			{
				throw std::invalid_argument("day can not larger than 28 when (year % 4)!=0");
			}
		}
		else
		{
			if (v > 29)
			{
				throw std::invalid_argument("day can not larger than 29 when (year % 4)==0");
			}
		}
	}
	if (m_month < 8)
	{
		if (m_month % 2)
		{
			if (v > 31)
			{
				throw std::invalid_argument("day can not larger than 31");
			}
		}
		else
		{
			if (v > 30)
			{
				throw std::invalid_argument("day can not larger than 30");
			}
		}
	}
	if (m_month > 8)
	{
		if (m_month % 2)
		{
			if (v > 30)
			{
				throw std::invalid_argument("day can not larger than 30");
			}
		}
		else
		{
			if (v > 31)
			{
				throw std::invalid_argument("day can not larger than 31");
			}
		}
	}
	m_day = v;
}

typename stdx::datetime::time_int_t stdx::datetime::hour() const
{
	return m_hour;
}

void stdx::datetime::hour(time_int_t v)
{
	if (v > 23)
	{
		throw std::invalid_argument("hour can not larger than 23");
	}
	m_hour = v;
}

typename stdx::datetime::time_int_t stdx::datetime::minute() const
{
	return m_minute;
}

void stdx::datetime::minute(time_int_t v)
{
	if (v>59)
	{
		throw std::invalid_argument("minute can not larger than 59");
	}
	m_minute = v;
}

stdx::datetime::time_int_t stdx::datetime::second() const
{
	return m_second;
}

void stdx::datetime::second(time_int_t v)
{
	if (v>59)
	{
		throw std::invalid_argument("second can not larger than 59");
	}
	m_second = v;
}

typename stdx::datetime::time_int_t stdx::datetime::millisecond() const
{
	return m_millisecond;
}

void stdx::datetime::millisecond(time_int_t v)
{
	if (v>999)
	{
		throw std::invalid_argument("millisecond can not larger than 999");
	}
	m_millisecond = v;
}

typename stdx::datetime::time_int_t stdx::datetime::day_of_month(time_int_t month, time_int_t year)
{
	if (month<1 || month>12)
	{
		throw std::invalid_argument("month can not larger than 12 or less than 1");
	}
	if (month == 2)
	{
		if (year % 4)
		{
			return 28;
		}
		else
		{
			return 29;
		}
	}
	if (month < 8)
	{
		if (month % 2)
		{
			return 31;
		}
		else
		{
			return 30;
		}
	}
	if (month % 2)
	{
		return 30;
	}
	return 31;
}

void stdx::datetime::operator+=(const time_span& span)
{
	time_int_t tmp = 0;

}

void stdx::datetime::operator-=(const time_span& span)
{
}

stdx::time_span stdx::datetime::operator-(const stdx::datetime& other) const
{
	
}