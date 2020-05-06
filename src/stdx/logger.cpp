#include <stdx/logger.h>
#include <string>
#include <time.h>

void stdx::_Logger::log(logger_level level, const stdx::string& msg) const
{
	const char* begin = nullptr;
	const char* end = "\033[39;49;0m";
	FILE* out = stdout;
	switch (level)
	{
	case stdx::logger_level::debug:
		begin = "\033[1m\033[40;37m[DEBUG]";
		break;
	case stdx::logger_level::info:
		begin = "\033[1m\033[40;32m[INFO ]";
		break;
	case stdx::logger_level::warn:
		begin = "\033[1m\033[40;33m[WARN ]";
		break;
	case stdx::logger_level::error:
		out = stderr;
		begin = "\033[1m\033[40;31m[ERROR]";
		break;
	case stdx::logger_level::fault:
		out = stderr;
		begin = "\033[1m\033[41;37m[FAULT]";
		break;
	default:
		return;
	}
	time_t tick = time(NULL);
	tm now;
#ifdef WIN32
	localtime_s(&now, &tick);
	fprintf(out, "%s[%d/%d/%d %d:%d:%d]%ls%s\n",begin,now.tm_year+1900,now.tm_mon,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec,msg.c_str(),end);
#else
	localtime_r(&tick,&now);
	fprintf(out, "%s[%d/%d/%d %d:%d:%d]%s%s\n", begin, now.tm_year+1900, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, msg.c_str(), end);
#endif
}

stdx::logger stdx::make_default_logger()
{
	return stdx::logger(std::make_shared<stdx::_Logger>());
}

#ifdef WIN32
stdx::_ANSIColorSupport::_ANSIColorSupport()
{
	HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (GetConsoleMode(std_out, &mode) != FALSE)
	{
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (SetConsoleMode(std_out, mode) == FALSE)
		{
#ifdef DEBUG
			::printf("启用ANSI 转义支持失败\n");
#endif
		}
	}
	else
	{
#ifdef DEBUG
		::printf("启用ANSI 转义支持失败\n");
#endif
	}
	HANDLE std_err = GetStdHandle(STD_ERROR_HANDLE);
	if (GetConsoleMode(std_err, &mode) != FALSE)
	{
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (SetConsoleMode(std_out, mode) == FALSE)
		{
#ifdef DEBUG
			::printf("启用ANSI 转义支持失败\n");
#endif
		}
	}
	else
	{
#ifdef DEBUG
		::printf("启用ANSI 转义支持失败\n");
#endif
	}
}

stdx::_ANSIColorSupport stdx::_ansi_color_support;
#endif