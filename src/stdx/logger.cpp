#include <stdx/logger.h>
#include <string>
#include <time.h>

#ifdef WIN32

void stdx::_Logger::debug(const char *str)
{
	SetConsoleTextAttribute(m_stderr, FOREGROUND_INTENSITY);
	time_t now = time(NULL);
	char buf[26];
	ctime_s(buf, sizeof(buf), &now);
	printf("[DEBUG][%s]%s\n",buf,str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::info(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 10);
	time_t now = time(NULL);
	char buf[26];
	ctime_s(buf, sizeof(buf), &now);
	printf("[INFO][%s]%s\n",buf,str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::warn(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 14);
	time_t now = time(NULL);
	char buf[26];
	ctime_s(buf, sizeof(buf), &now);
	printf("[WARN][%s]%s\n",buf,str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::error(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 12|FOREGROUND_INTENSITY);
	time_t now = time(NULL);
	char buf[26];
	ctime_s(buf, sizeof(buf), &now);
	fprintf(stderr,"[ERROR][%s]%s\n",buf,str);
	SetConsoleTextAttribute(m_stderr,0x07);
}
#else
void stdx::_Logger::debug(const char* str)
{
	time_t now = time(NULL);
	char buf[26];
	ctime_r(&now,buf);
	printf("\033[1m\033[40;37m[DEBUG][%s]%s\033[39;49;0m\n",buf,str);
}
void stdx::_Logger::info(const char* str)
{ 
	time_t now = time(NULL);
	char buf[26];
	ctime_r(&now, buf);
	printf("\033[1m\033[40;32m[INFO][%s]%s\033[39;49;0m\n",buf, str);
}

void stdx::_Logger::warn(const char* str)
{
	time_t now = time(NULL);
	char buf[26];
	ctime_r(&now, buf);
	printf("\033[1m\033[40;33m[WARN][%s]%s\033[39;49;0m\n",buf, str);
}

void stdx::_Logger::error(const char* str)
{
	time_t now = time(NULL);
	char buf[26];
	ctime_r(&now, buf);
	fprintf(stderr,"\033[1m\033[40;31m[ERROR][%s]%s\033[39;49;0m\n", buf,str);
}
#endif

stdx::logger stdx::make_default_logger()
{
	return stdx::logger(std::make_shared<stdx::_Logger>());
}