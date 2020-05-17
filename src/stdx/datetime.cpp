#include <stdx/datetime.h>
#ifdef LINUX
#include <sys/timeb.h>
#endif

#ifdef WIN32
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); \
						}
#define _ThrowWSAError 	auto _ERROR_CODE = WSAGetLastError(); \
						if(_ERROR_CODE != WSA_IO_PENDING)\
						{\
							throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()));\
						}
#else
#define _ThrowLinuxError auto _ERROR_CODE = errno;\
						 throw std::system_error(std::error_code(_ERROR_CODE,std::system_category())); 
#endif

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

stdx::datetime::datetime(const tm& t)
	:m_year(((t.tm_year+1900) >= 1970)?(t.tm_year+1900):1970)
	,m_month(t.tm_mon)
	,m_day(t.tm_mday)
	,m_hour(t.tm_hour)
	,m_minute(t.tm_min)
	,m_second(t.tm_sec)
	,m_millisecond(0)
{}

stdx::datetime& stdx::datetime::operator=(const stdx::datetime& other)
{
	stdx::datetime tmp(other);
	stdx::atomic_copy(*this, std::move(tmp));
	return *this;
}

stdx::datetime& stdx::datetime::operator=(stdx::datetime &&other) noexcept
{
	m_year = std::move(other.m_year);
	m_month = std::move(other.m_month);
	m_day = std::move(other.m_day);
	m_hour = std::move(other.m_hour);
	m_minute = std::move(other.m_minute);
	m_second = std::move(other.m_second);
	m_millisecond = std::move(other.m_millisecond);
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
	m_millisecond += span.millisecond;
	if (m_millisecond > 999)
	{
		while (m_millisecond > 999)
		{
			tmp += 1;
			m_millisecond -= 1000;
		}
	}
	if (tmp != 0)
	{
		m_second += tmp;
		tmp = 0;
	}
	m_second += span.second;
	if (m_second > 59)
	{
		while (m_second > 59)
		{
			tmp += 1;
			m_second -= 60;
		}
	}
	if (tmp!=0)
	{
		m_minute += tmp;
		tmp = 0;
	}
	m_minute += span.minute;
	if (m_minute > 59)
	{
		while (m_minute > 59)
		{
			tmp += 1;
			m_minute -= 60;
		}
	}
	if (tmp!=0)
	{
		m_hour += tmp;
		tmp = 0;
	}
	m_hour += span.hour;
	if (m_hour > 23)
	{
		while (m_hour > 23)
		{
			m_hour -= 24;
			tmp += 1;
		}
	}
	if (tmp!=0)
	{
		m_day += tmp;
	}
	m_day += span.day;
	while (m_day > day_of_month(m_month,m_year))
	{
		m_day -= day_of_month(m_month,m_year);
		m_month += 1;
		if (m_month > 12)
		{
			m_month -= 12;
			m_year += 1;
		}
	}
	m_month += span.month;
	if (m_month > 12)
	{
		while (m_month > 12)
		{
			m_month -= 12;
			m_year += 1;
		}
	}
	m_year += span.year;
}

void stdx::datetime::operator-=(const time_span& span)
{
	int32_t t1, t2;
	t1 = (int32_t)m_year * 12 + m_month;
	t2 = (int32_t)span.year * 12 + span.month;
	if (t2 >t1)
	{
		bezero();
		return;
	}
	t1 -= t2;
	m_year = 0;
	while (t1 > 12)
	{
		t1 -= 12;
		m_year += 1;
	}
	if (m_year < 1970)
	{
		bezero();
		return;
	}
	m_month = t1;
	if (m_day < span.day)
	{
		t1 = m_day - span.day;
		while (t1 < 0)
		{
			if (m_month > 1)
			{
				time_int_t day = day_of_month(m_month, m_year);
				m_month -= 1;
				t1 += day;
			}
			else
			{
				m_year -= 1;
				m_month += 11;
				t1 += 31;
			}
		}
		m_day = t1;
		if (m_year < 1970)
		{
			bezero();
			return;
		}
	}
	else
	{
		m_day -= span.day;
	}
	t1 = m_hour * 3600 + m_minute * 60 + m_second;
	t2 = span.hour * 3600 + span.minute * 60 + span.second;
	if (t1 < t2)
	{
		t1 -= t2;
		while (t1 < 0)
		{
			if (m_day > 1)
			{
				t1 += 24 * 3600;
				m_day -= 1;
			}
			else
			{
				if (m_month > 1)
				{
					time_int_t day = day_of_month(m_month, m_year);
					m_month -= 1;
					m_day += day - 1;
					t1 += 24 * 3600;
				}
				else
				{
					m_year -= 1;
					m_month += 11;
					m_day += 30;
					t1 += 24 * 3600;
				}
			}
		}
		if (m_year < 1970)
		{
			bezero();
			return;
		}
	}
	else
	{
		t1 -= t2;
	}
	t1 *= 1000;
	m_hour = 0;
	m_minute = 0;
	m_second = 0;
	t2 = m_millisecond - span.millisecond;
	t1 += t2;
	t2 = 0;
	while (t1 > 999)
	{
		t1 -= 1000;
		t2 += 1;
	}
	m_millisecond = t1;
	t1 = 0;
	while (t2 > 59)
	{
		t1 += 1;
		t2 -= 60;
	}
	m_second = t2;
	t2 = 0;
	while (t1 > 59)
	{
		t2 += 1;
		t1 -= 60;
	}
	m_minute = t1;
	m_hour = t2;
}

//stdx::time_span stdx::datetime::operator-(const stdx::datetime& other) const
//{
//	stdx::time_span span;
//
//	if (this->operator>(other))
//	{
//		time_t t1 = (time_t)*this,t2=(time_t)other;
//	}
//	else if(this->operator<(other))
//	{
//		stdx::datetime tm(other);
//		stdx::time_span tmp;
//		tmp.year = m_year;
//		tmp.month = m_month;
//		tmp.day = m_day;
//		tmp.hour = m_hour;
//		tmp.minute = m_minute;
//		tmp.second = m_second;
//		tmp.millisecond = m_millisecond;
//		tm -= tmp;
//		span = tmp;
//	}
//	return span;
//}

stdx::datetime::operator tm() const
{
	tm tmp;
	tmp.tm_year = m_year - 1900;
	tmp.tm_mon = m_month;
	tmp.tm_mday = m_day;
	tmp.tm_hour = m_hour;
	tmp.tm_min = m_minute;
	tmp.tm_sec = m_second;
	return tmp;
}

stdx::datetime::operator time_t() const
{
	tm t = (tm)*this;
	time_t tmp = mktime(&t);
	return tmp;
}

void stdx::datetime::bezero()
{
	m_year = 1970;
	m_month = 1;
	m_day = 1;
	m_hour = 0;
	m_minute = 0;
	m_second = 0;
	m_millisecond = 0;
}

stdx::string stdx::datetime::to_string(const typename stdx::string::char_t* format) const
{
	stdx::string str = format;
	str.replace(U("%year"), stdx::to_string(m_year));
	str.replace(U("%mon"), stdx::to_string(m_month));
	if (m_day < 10)
	{
		stdx::string tmp(U("0"));
		tmp.append(stdx::to_string(m_day));
		str.replace(U("%day"),tmp);
	}
	else
	{
		str.replace(U("%day"), stdx::to_string(m_day));
	}
	if (m_hour < 10)
	{
		stdx::string tmp(U("0"));
		tmp.append(stdx::to_string(m_hour));
		str.replace(U("%hour"), tmp);
	}
	else
	{
		str.replace(U("%hour"), stdx::to_string(m_hour));
	}
	if (m_minute < 10)
	{
		stdx::string tmp(U("0"));
		tmp.append(stdx::to_string(m_minute));
		str.replace(U("%min"), tmp);
	}
	else
	{
		str.replace(U("%min"), stdx::to_string(m_minute));
	}
	if (m_second < 10)
	{
		stdx::string tmp(U("0"));
		tmp.append(stdx::to_string(m_second));
		str.replace(U("%sec"), tmp);
	}
	else
	{
		str.replace(U("%sec"), stdx::to_string(m_second));
	}
	str.replace(U("%msec"),stdx::to_string(m_millisecond));
	return str;
}

stdx::datetime stdx::datetime::now()
{
#ifdef WIN32
	SYSTEMTIME time;
	GetLocalTime(&time);
	stdx::datetime tmp;
	tmp.m_year = time.wYear;
	tmp.m_month = time.wMonth;
	tmp.m_day = time.wDay;
	tmp.m_hour = time.wHour;
	tmp.m_minute = time.wMinute;
	tmp.m_second = time.wSecond;
	tmp.m_millisecond = time.wMilliseconds;
	return tmp;
#else
	struct timeb tp;
	ftime(&tp);
	tm t;
	localtime_r(&(tp.time),&t);
	stdx::datetime tmp(t);
	tmp.m_millisecond = tp.millitm;
	return tmp;
#endif
}

stdx::datetime stdx::datetime::now_utc()
{
#ifdef WIN32
	SYSTEMTIME time;
	GetSystemTime(&time);
	stdx::datetime tmp;
	tmp.m_year = time.wYear;
	tmp.m_month = time.wMonth;
	tmp.m_day = time.wDay;
	tmp.m_hour = time.wHour;
	tmp.m_minute = time.wMinute;
	tmp.m_second = time.wSecond;
	tmp.m_millisecond = time.wMilliseconds;
	return tmp;
#else
	struct timeb tp;
	ftime(&tp);
	tm t;
	localtime_r(&(tp.time), &t);
	stdx::datetime tmp(t);
	tmp.m_millisecond = tp.millitm;
	stdx::time_span span;
	span.minute = tp.timezone;
	tmp += span;
	return tmp;
#endif
}

stdx::week_day stdx::datetime::week_day() const
{
	uint32_t day = m_day;
	for (size_t i = m_month; i != 1; i--)
	{
		day += day_of_month(m_month, m_year);
	}
	if (m_year < 1972)
	{
		day += 365;
	}
	else
	{
		day += 365;
		uint32_t tmp = m_year-1971;
		if (tmp!=0)
		{
			if (tmp % 4)
			{
				uint32_t t = tmp / 4;
				day += t * (365 * 3 + 366);
				tmp %= 4;
				day += t * 365;
			}
			else
			{
				tmp /= 4;
				day += tmp * (365 * 3 + 366);
			}
		}
	}
	day += 4;
	day %= 7;
	switch (day)
	{
	case 0:
		return stdx::week_day::sun;
	case 1:
		return stdx::week_day::mon;
	case 2:
		return stdx::week_day::tues;
	case 3:
		return stdx::week_day::wed;
	case 4:
		return stdx::week_day::thur;
	case 5:
		return stdx::week_day::fri;
	case 6:
		return stdx::week_day::sat;
	default:
		throw std::logic_error("unkonw week day");
	}
}

stdx::string stdx::to_day_name(stdx::week_day day)
{
	switch (day)
	{
	case stdx::week_day::sun:
		return U("Sun");
	case stdx::week_day::mon:
		return U("Mon");
	case stdx::week_day::tues:
		return U("Tue");
	case stdx::week_day::wed:
		return U("Wed");
	case stdx::week_day::thur:
		return U("Thu");
	case stdx::week_day::fri:
		return U("Fri");
	case stdx::week_day::sat:
		return U("Sat");
	default:
		throw std::logic_error("Unkonw week day");
	}
}

stdx::string stdx::to_month_name(uint16_t month)
{
	switch (month)
	{
	case 1:
		return U("Jan");
	case 2:
		return U("Feb");
	case 3:
		return U("Mar");
	case 4:
		return U("Apr");
	case 5:
		return U("May");
	case 6:
		return U("Jun");
	case 7:
		return U("Jul");
	case 8:
		return U("Aug");
	case 9:
		return U("Sep");
	case 10:
		return U("Oct");
	case 11:
		return U("Nov");
	case 12:
		return U("Dec");
	default:
		throw std::logic_error("Unkonw month");
	}
}

uint16_t stdx::month_name_to_time_int(stdx::string&& name)
{
	name.lower();
	if (name == U("jan"))
	{
		return 1;
	}
	else if (name == U("feb"))
	{
		return 2;
	}
	else if(name == U("mar"))
	{
		return 3;
	}
	else if (name == U("apr"))
	{
		return 4;
	}
	else if(name == U("may"))
	{
		return 5;
	}
	else if(name == U("jun"))
	{
		return 6;
	}
	else if (name == U("jul"))
	{
		return 7;
	}
	else if(name == U("aug"))
	{
		return 8;
	}
	else if(name == U("sep"))
	{
		return 9;
	}
	else if(name == U("oct"))
	{
		return 10;
	}
	else if (name == U("nov"))
	{
		return 11;
	}
	else if(name == U("dec"))
	{
		return 12;
	}
	else
	{
		throw std::invalid_argument("unkonw month name");
	}
}

uint16_t stdx::month_name_to_time_int(const stdx::string& name)
{
	stdx::string tmp(name);
	return month_name_to_time_int(std::move(tmp));
}

uint64_t stdx::get_tick_count()
{
#ifdef WIN32
	return GetTickCount64();
#else
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
	{
		_ThrowLinuxError
	}
	return (ts.tv_sec * 1000 + ts.tv_nsec/1000000);
#endif
}