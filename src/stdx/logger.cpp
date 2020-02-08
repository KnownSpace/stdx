#include <stdx/logger.h>
#include <string>

#ifdef WIN32

void stdx::_Logger::debug(const char *str)
{
	SetConsoleTextAttribute(m_stderr, FOREGROUND_INTENSITY);
	printf("[DEBUG]%s\n", str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::info(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 10);
	printf("[INFO]%s\n", str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::warn(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 14);
	printf("[WARN]%s\n",str);
	SetConsoleTextAttribute(m_stderr, 0x07);
}

void stdx::_Logger::error(const char* str)
{
	SetConsoleTextAttribute(m_stderr, 12|FOREGROUND_INTENSITY);
	fprintf(stderr,"[ERROR]%s\n", str);
	SetConsoleTextAttribute(m_stderr,0x07);
}
#else
void stdx::_Logger::debug(const char* str)
{
	printf("\033[1m\033[40;37m[DEBUG]%s\033[39;49;0m\n",str);
}
void stdx::_Logger::info(const char* str)
{ 
	printf("\033[1m\033[40;32m[INFO]%s\033[39;49;0m\n", str);
}

void stdx::_Logger::warn(const char* str)
{
	printf("\033[1m\033[40;33m[WARN]%s\033[39;49;0m\n", str);
}

void stdx::_Logger::error(const char* str)
{
	fprintf(stderr,"\033[1m\033[40;31m[ERROR]%s\033[39;49;0m\n", str);
}
#endif

stdx::logger stdx::make_default_logger()
{
	return stdx::logger(std::make_shared<stdx::_Logger>());
}