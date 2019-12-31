#pragma once
#include <stdx/env.h>
#include <string>
namespace stdx
{
	template<typename _T>
	struct is_basic_string
	{
		enum 
		{
			value = 0
		};
	};

	template<typename _TChar>
	struct is_basic_string<std::basic_string<_TChar>>
	{
		enum 
		{
			value = 1
		};
	};

	template<typename _String,typename _Container,class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void _SpitStr(_String &str,const _String &chars, _Container &container)
	{
		if (chars.empty())
		{
			throw std::invalid_argument("argument chars can not be empty");
		}
		size_t pos = str.find(chars);
		if (pos == _String::npos)
		{
			container.push_back(str);
		}
		else
		{
			if (pos != 0)
			{
				container.push_back(str.substr(0,pos));
			}
			if (pos != (str.size()-1))
			{
				_String substr(std::move(str.substr(pos + chars.size(), str.size() - 1)));
				_SpitStr(substr, chars, container);
			}
		}
	}

	template<typename _Container,typename _String=std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void spit_string(_String &str, const _String &chars,_Container &container)
	{
		return _SpitStr(str, chars, container);
	}

	//template<typename _Container, typename _String = std::string>
	//inline void spit_string(_String &str, _String &&chars, _Container &container)
	//{
	//	_String c = chars;
	//	return _SpitStr(str,c, container);
	//}

	template<typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void replace_string(_String &str,const _String &target,const _String &val)
	{
		size_t pos = str.find(target);
		if (pos == _String::npos)
		{
			return;
		}
		else
		{
			str.replace(pos,target.size(), val);
			return replace_string(str, target, val);
		}
	}

	template<typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void html_decode(_String &str)
	{
		replace_string<_String>(str, "&quot;", "\"");
		replace_string<_String>(str, "&#34;", "\"");
		replace_string<_String>(str, "&amp;", "&");
		replace_string<_String>(str, "&#38;", "&");
		replace_string<_String>(str, "&lt;", "<");
		replace_string<_String>(str, "&#60;", "<");
		replace_string<_String>(str, "&gt;", ">");
		replace_string<_String>(str, "&#62;", ">");
		replace_string<_String>(str, "&#39;", "'");
	}

	template<typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void html_encode(_String &str)
	{
		replace_string<_String>(str, "\"", "&quot;");
		replace_string<_String>(str, "&", "&amp;");
		replace_string<_String>(str, "<", "&lt;");
		replace_string<_String>(str, ">", "&gt;");
		replace_string<_String>(str, "'", "&#39;");
	}

#ifdef WIN32
#define U(x) u8##x
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define _ThrowWinError auto _ERROR_CODE = GetLastError(); \
						LPVOID _MSG;\
						if(_ERROR_CODE != ERROR_IO_PENDING) \
						{ \
							if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,_ERROR_CODE,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &_MSG,0,NULL))\
							{ \
								throw std::runtime_error((char*)_MSG);\
							}else \
							{ \
								std::string _ERROR_MSG("windows system error:");\
								_ERROR_MSG.append(std::to_string(_ERROR_CODE));\
								throw std::system_error(std::error_code(_ERROR_CODE,std::system_category()),_ERROR_MSG.c_str()); \
							} \
						}\

	using unicode_string = std::wstring;
	struct code_page
	{
		enum 
		{
			ansi = CP_ACP,
			utf8 = CP_UTF8
		};
	};

	template<typename _String = std::string,typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_utf8(const _UnicodeString &src)
	{
		using char_t = typename _String::value_type;
		DWORD size = WideCharToMultiByte(stdx::code_page::utf8, NULL, src.c_str(), -1, NULL, 0, NULL, FALSE);
		char *buf = (char*)calloc(size, sizeof(char));
		if (!(WideCharToMultiByte(stdx::code_page::utf8,NULL,src.c_str(),-1,buf,size,NULL,FALSE)))
		{
			_ThrowWinError
		}
		_String des = (char_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_ansi(const _UnicodeString &src)
	{
		using char_t = typename _String::value_type;
		DWORD size = WideCharToMultiByte(stdx::code_page::ansi, NULL, src.c_str(), -1, NULL, 0, NULL, FALSE);
		char *buf = (char*)calloc(size, sizeof(char));
		if (!(WideCharToMultiByte(stdx::code_page::ansi, NULL, src.c_str(), -1, buf, size, NULL, FALSE)))
		{
			_ThrowWinError
		}
		_String des = (char_t*)buf;
		free(buf);
		return des;
	}

	template<typename _UnicodeString = stdx::unicode_string,typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString utf8_to_unicode(const _String &src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		DWORD size = MultiByteToWideChar(stdx::code_page::utf8, NULL, src.c_str(), -1, NULL, 0);
		wchar_t *buf = (wchar_t*)calloc(size, sizeof(wchar_t));
		if (!(MultiByteToWideChar(stdx::code_page::utf8, NULL, src.c_str(), -1, buf, size)))
		{
			_ThrowWinError
		}
		_UnicodeString des = (uchar_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String utf8_to_ansi(const _String &src)
	{
		stdx::unicode_string temp = stdx::utf8_to_unicode(src);
		return stdx::unicode_to_ansi(temp);
	}

	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString ansi_to_unicode(const _String &src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		DWORD size = MultiByteToWideChar(stdx::code_page::ansi, NULL, src.c_str(), -1, NULL, 0);
		wchar_t *buf = (wchar_t*)calloc(size, sizeof(wchar_t));
		if (!(MultiByteToWideChar(stdx::code_page::ansi, NULL, src.c_str(), -1, buf, size)))
		{
			_ThrowWinError
		}
		_UnicodeString des = (uchar_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String ansi_to_utf8(const _String &src)
	{
		stdx::unicode_string temp = stdx::ansi_to_unicode(src);
		return stdx::unicode_to_utf8(temp);
	}

	
#undef _ThrowWinError
#endif
#ifdef LINUX
#include <errno.h>
#include <string.h>
#include <string>
#ifndef ANSI_CODE
#define ANSI_CODE "GBK"
#endif // !ANSI_CODE

#define U(x) x
#define _ThrowLinuxError auto _ERROR_CODE = errno; \
	throw std::system_error(std::error_code(_ERROR_CODE, std::system_category()), strerror(_ERROR_CODE)); \

#include <iconv.h>

	using unicode_string = std::basic_string<int_16>;
	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_utf8(const _UnicodeString &src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open("UTF-8", "UCS-2LE");
		char *buf = (char*)src.c_str();
		size_t size = src.size()*2;
		char *out = (char*)calloc(size,sizeof(char));
		char *p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_ansi(const _UnicodeString &src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open(ANSI_CODE, "UCS-2LE");
		if (conv == (iconv_t)-1)
		{
			_ThrowLinuxError
		}
		char *buf = (char*)src.c_str();
		size_t size = src.size() * 2;
		char *out = (char*)calloc(size,sizeof(char));
		char *p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString utf8_to_unicode(const _String &src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		iconv_t conv = iconv_open("UCS-2LE", "UTF-8");
		size_t size = src.size();
		size_t out_size = size + (size%2);
		char *buf = (char*)src.c_str();
		char *out = (char*)calloc(out_size, sizeof(char));
		char *p = out;
		iconv(conv, &buf, &size, &out, &out_size);
		_UnicodeString des=(uchar_t*)p;
		free(p);
		iconv_close(conv);
		return des;
	}
	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String utf8_to_ansi(const _String &src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open(ANSI_CODE, "UTF-8");
		char *buf = (char*)src.c_str();
		size_t size = src.size();
		char *out = (char*)calloc(size, sizeof(char));
		char *p = out;
		if (iconv(conv, &buf, &size, &out, &size) == -1);
		{
			free(out);
			iconv_close(conv);
			_ThrowLinuxError
		}
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}
	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString ansi_to_unicode(const _String &src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		iconv_t conv = iconv_open("UCS-2LE", ANSI_CODE);
		char *buf = (char*)src.c_str();
		size_t size = src.size();
		size_t out_size = size + (size % 2);
		char *out = (char*)calloc(out_size, sizeof(char));
		char *p = out;
		iconv(conv, &buf, &size, &out, &out_size);
		iconv_close(conv);
		_UnicodeString des = (uchar_t*)p;
		free(p);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String ansi_to_utf8(const _String &src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open("UTF-8", ANSI_CODE);
		if (conv == (iconv_t)-1)
		{
			_ThrowLinuxError
		}
		char *buf = (char*)src.c_str();
		size_t size = src.size();
		char *out = (char*)calloc(size, sizeof(char));
		char *p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

#undef _ThrowLinuxError
#endif
}