#pragma once
#include <stdx/env.h>
#include <string>
#include <list>

namespace stdx
{
	template<typename _T>
	struct is_basic_string
	{
		constexpr static bool value = false;
	};

	template<typename _TChar>
	struct is_basic_string<std::basic_string<_TChar>>
	{
		constexpr static bool value = true;
	};

	template<typename _String, typename _Container, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void _SpitStr(const _String& str, const _String& chars, _Container& container)
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
				container.push_back(str.substr(0, pos));
			}
			if (pos != (str.size() - 1))
			{
				_String substr(std::move(str.substr(pos + chars.size(), str.size() - 1)));
				_SpitStr(substr, chars, container);
			}
		}
	}

	template<typename _Container, typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void split_string(const _String& str, const _String& chars, _Container& container)
	{
		return _SpitStr(str, chars, container);
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline void replace_string(_String& str, const _String& target, const _String& val)
	{
		size_t pos = str.find(target);
		if (pos == _String::npos)
		{
			return;
		}
		else
		{
			str.replace(pos, target.size(), val);
			return replace_string(str, target, val);
		}
	}

#ifdef WIN32
#define __U(x) L##x
#define U(x) __U(x)
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

	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_utf8(const _UnicodeString& src)
	{
		using char_t = typename _String::value_type;
		DWORD size = WideCharToMultiByte(stdx::code_page::utf8, NULL, src.c_str(), -1, NULL, 0, NULL, FALSE);
		char* buf = (char*)calloc(size, sizeof(char));
		if (buf == nullptr)
		{
			throw std::bad_alloc();
		}
		if (!(WideCharToMultiByte(stdx::code_page::utf8, NULL, src.c_str(), -1, buf, size, NULL, FALSE)))
		{
			_ThrowWinError
		}
		_String des = (char_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_ansi(const _UnicodeString& src)
	{
		using char_t = typename _String::value_type;
		DWORD size = WideCharToMultiByte(stdx::code_page::ansi, NULL, src.c_str(), -1, NULL, 0, NULL, FALSE);
		char* buf = (char*)calloc(size, sizeof(char));
		if (buf == nullptr)
		{
			throw std::bad_alloc();
		}
		if (!(WideCharToMultiByte(stdx::code_page::ansi, NULL, src.c_str(), -1, buf, size, NULL, FALSE)))
		{
			_ThrowWinError
		}
		_String des = (char_t*)buf;
		free(buf);
		return des;
	}

	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString utf8_to_unicode(const _String& src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		DWORD size = MultiByteToWideChar(stdx::code_page::utf8, NULL, src.c_str(), -1, NULL, 0);
		wchar_t* buf = (wchar_t*)calloc(size, sizeof(wchar_t));
		if (buf == nullptr)
		{
			throw std::bad_alloc();
		}
		if (!(MultiByteToWideChar(stdx::code_page::utf8, NULL, src.c_str(), -1, buf, size)))
		{
			_ThrowWinError
		}
		_UnicodeString des = (uchar_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String utf8_to_ansi(const _String& src)
	{
		stdx::unicode_string temp = stdx::utf8_to_unicode(src);
		return stdx::unicode_to_ansi(temp);
	}

	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString ansi_to_unicode(const _String& src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		DWORD size = MultiByteToWideChar(stdx::code_page::ansi, NULL, src.c_str(), -1, NULL, 0);
		wchar_t* buf = (wchar_t*)calloc(size, sizeof(wchar_t));
		if (buf == nullptr)
		{
			throw std::bad_alloc();
		}
		if (!(MultiByteToWideChar(stdx::code_page::ansi, NULL, src.c_str(), -1, buf, size)))
		{
			_ThrowWinError
		}
		_UnicodeString des = (uchar_t*)buf;
		free(buf);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String ansi_to_utf8(const _String& src)
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

	using unicode_string = std::basic_string<int16_t>;
	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_utf8(const _UnicodeString& src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open("UTF-8", "UCS-2LE");
		char* buf = (char*)src.c_str();
		size_t size = src.size() * 2;
		char* out = (char*)calloc(size, sizeof(char));
		char* p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

	template<typename _String = std::string, typename _UnicodeString, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _String unicode_to_ansi(const _UnicodeString& src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open(ANSI_CODE, "UCS-2LE");
		if (conv == (iconv_t)-1)
		{
			_ThrowLinuxError
		}
		char* buf = (char*)src.c_str();
		size_t size = src.size() * 2;
		char* out = (char*)calloc(size, sizeof(char));
		char* p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

	template<typename _UnicodeString = stdx::unicode_string, typename _String, class = typename  std::enable_if<stdx::is_basic_string<_String>::value && stdx::is_basic_string<_UnicodeString>::value>::type>
	inline _UnicodeString utf8_to_unicode(const _String& src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		iconv_t conv = iconv_open("UCS-2LE", "UTF-8");
		size_t size = src.size();
		size_t out_size = size + (size % 2);
		char* buf = (char*)src.c_str();
		char* out = (char*)calloc(out_size, sizeof(char));
		char* p = out;
		iconv(conv, &buf, &size, &out, &out_size);
		_UnicodeString des = (uchar_t*)p;
		free(p);
		iconv_close(conv);
		return des;
	}
	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String utf8_to_ansi(const _String& src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open(ANSI_CODE, "UTF-8");
		char* buf = (char*)src.c_str();
		size_t size = src.size();
		char* out = (char*)calloc(size, sizeof(char));
		char* p = out;
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
	inline _UnicodeString ansi_to_unicode(const _String& src)
	{
		using uchar_t = typename _UnicodeString::value_type;
		iconv_t conv = iconv_open("UCS-2LE", ANSI_CODE);
		char* buf = (char*)src.c_str();
		size_t size = src.size();
		size_t out_size = size + (size % 2);
		char* out = (char*)calloc(out_size, sizeof(char));
		char* p = out;
		iconv(conv, &buf, &size, &out, &out_size);
		iconv_close(conv);
		_UnicodeString des = (uchar_t*)p;
		free(p);
		return des;
	}

	template<typename _String = std::string, class = typename  std::enable_if<stdx::is_basic_string<_String>::value>::type>
	inline _String ansi_to_utf8(const _String& src)
	{
		using char_t = typename _String::value_type;
		iconv_t conv = iconv_open("UTF-8", ANSI_CODE);
		if (conv == (iconv_t)-1)
		{
			_ThrowLinuxError
		}
		char* buf = (char*)src.c_str();
		size_t size = src.size();
		char* out = (char*)calloc(size, sizeof(char));
		char* p = out;
		iconv(conv, &buf, &size, &out, &size);
		iconv_close(conv);
		_String des = (char_t*)p;
		free(p);
		return des;
	}

#undef _ThrowLinuxError
#endif

	extern char switch_to_base64_char(unsigned char byte);

	extern char base64_char_to_char(char ch);

	extern std::string to_base64_string(const std::string& data);

	extern std::string from_base64_string(const std::string& base64_str);

	extern std::string url_decode(const std::string& str);

	extern std::string url_encode(const std::string& str);

	extern char hex_to_char(unsigned char byte);

	struct string;
}

#ifdef WIN32
extern std::wostream& operator<<(std::wostream& out, const stdx::string& str);
extern std::wistream& operator>>(std::wistream& in, stdx::string& str);
#else
extern std::ostream& operator<<(std::ostream& out, const stdx::string& str);
extern std::istream& operator>>(std::istream& in, stdx::string& str);
#endif

namespace stdx
{
	class buffer;

	struct string
	{
		friend struct std::hash<stdx::string>;
	public:
#pragma region type_def
#ifdef WIN32
		using string_t = std::wstring;
#else
		using string_t = std::string;
#endif
		using char_t = typename string_t::value_type;
		using const_char_t = const char_t;
		using iterator_t = typename string_t::iterator;
		using size_type = typename string_t::size_type;
		using const_iterator_t = typename string_t::const_iterator;
		using reverse_iterator_t = typename string_t::reverse_iterator;
		using const_reverse_iterator_t = typename string_t::const_reverse_iterator;
#pragma endregion
		string();

		string(const char_t* str);

		string(const string_t& str);

		string(const stdx::string& other);

		string(stdx::string&& other) noexcept;

		~string() = default;
		
		stdx::string& operator=(const stdx::string& other);

		stdx::string& operator=(stdx::string&& other) noexcept;

		stdx::string& operator=(const char_t* str);

		static const size_type npos = -1;

		char_t &at(const size_type &pos);
		char_t at(const size_type &pos) const;

		char_t &front();
		char_t front() const;

		char_t &back();
		char_t back() const;

		const char_t *data() const;
		const char_t *c_str() const;

		iterator_t begin();
		const_iterator_t cbegin() const;

		iterator_t end();
		const_iterator_t cend() const;

		reverse_iterator_t rbegin();
		const_reverse_iterator_t crbegin() const;

		reverse_iterator_t rend();
		const_reverse_iterator_t crend() const;

		bool empty() const;

		size_type size() const;

		size_type max_size() const;

		size_type capacity() const;

		void shrink_to_fit();

		void clear();

		void insert(const size_type &index,const char_t *str);
		void insert(const size_type& index, const char_t* str,const size_type &count);

		void erase(size_type index);
		void erase(size_type index, size_type count);
		iterator_t erase(iterator_t position);
		iterator_t erase(iterator_t begin, iterator_t end);
		void erase(const stdx::string& target);
		void erase(typename stdx::string::char_t ch);
		void erase_once(const stdx::string& target);
		void erase_once(typename stdx::string::char_t ch);
		void erase_front();
		void erase_back();

		void push_back(const char_t &ch);

		void append(const char_t *str);
		void append(const char_t *str,const size_type &size);
		void append(const stdx::string&other);
		template<typename InputIt>
		void append(InputIt first,InputIt last)
		{
			m_data.append(first,last);
		}

		bool begin_with(const char_t *str) const;
		bool begin_with(const char_t &ch) const;
		bool begin_with(const stdx::string&other) const;

		bool end_with(const char_t* str) const;
		bool end_with(char_t ch) const;
		bool end_with(const stdx::string& other) const;

		stdx::string &replace(const stdx::string &dest,const stdx::string&text);
		stdx::string &replace(const size_type &pos,const size_type &count,const stdx::string&text);
		stdx::string &replace(iterator_t begin,iterator_t end,const stdx::string&text);

		stdx::string replace(const stdx::string& dest, const stdx::string& text) const;
		stdx::string replace(const size_type& pos, const size_type& count, const stdx::string& text) const;
		stdx::string replace(iterator_t begin, iterator_t end, const stdx::string& text) const;

		stdx::string substr(size_type pos = 0,size_type count = npos) const;
		
		void swap(stdx::string& other);

		size_type find(const stdx::string&str, size_type pos = 0) const;
		size_type find(const char_t *s, size_type pos = 0) const;
		size_type find(const char_t &ch, size_type pos = 0) const;

		size_type rfind(const stdx::string& str, size_type pos = 0) const;
		size_type rfind(const char_t* s, size_type pos = 0) const;
		size_type rfind(const char_t &ch, size_type pos = 0) const;

		size_type find_first_of(const stdx::string& str, size_type pos = 0) const;
		size_type find_first_of(const char_t* s, size_type pos, size_type count) const;
		size_type find_first_of(const char_t* s, size_type pos = 0) const;
		size_type find_first_of(const char_t &ch, size_type pos = 0) const;

		size_type find_last_of(const stdx::string& str, size_type pos = 0) const;
		size_type find_last_of(const char_t* s, size_type pos, size_type count) const;
		size_type find_last_of(const char_t* s, size_type pos = 0) const;
		size_type find_last_of(const char_t &ch, size_type pos = 0) const;

		bool equal(const stdx::string& other) const;
		bool equal(const char_t *str) const;

		string_t to_std_string() const;

		std::string to_u8_string() const;

		std::string to_native_string() const;

		std::string to_u8_base64_string() const;

		std::string to_native_base64_string() const;

		static stdx::string from_native_string(const std::string &str);

		static stdx::string from_u8_string(const std::string &str);

		static stdx::string from_u8_base64_string(const std::string &base64_str);

		static stdx::string from_buffer(const stdx::buffer &buf);

		static stdx::string from_native_base64_string(const std::string &base64_str);

		void html_encode();

		void html_decode();

		void u8_url_encode();

		void u8_url_decode();

		std::string to_u8_url_encode() const;

		static stdx::string from_u8_url_decode(const std::string &url_str);

		void native_url_encode();

		void native_url_decode();

		std::string to_native_url_encode() const;

		static stdx::string from_native_url_decode(const std::string& url_str);

		bool is_lower() const;

		stdx::string &lower();

		stdx::string lower() const;

		bool is_upper() const;

		stdx::string &upper();

		stdx::string upper() const;

		template<typename _Container = std::list<stdx::string>>
		_Container split(const stdx::string &text) const
		{
			_Container cont;
			stdx::split_string<_Container, string_t>(m_data,text.m_data,cont);
			return cont;
		}

		bool operator==(const stdx::string& other) const
		{
			return equal(other);
		}

		bool operator==(const char_t* str) const
		{
			return equal(str);
		}

		bool operator!=(const stdx::string& other) const
		{
			return !equal(other);
		}

		bool operator!=(const char_t* str) const
		{
			return !equal(str);
		}

		stdx::string& operator+=(const stdx::string& other)
		{
			m_data.append(other.m_data);
			return *this;
		}

		stdx::string operator+(const stdx::string &other)
		{
			return stdx::string(m_data + other.m_data);
		}

		char_t &operator[](size_t index)
		{
			return m_data[index];
		}

		char_t operator[](size_t index) const
		{
			return m_data[index];
		}

		static char_t hex_to_uchar(unsigned char byte);

		int32_t to_int32() const;
		int64_t to_int64() const;

		uint32_t to_uint32() const;
		uint64_t to_uint64() const;

		double to_double() const;
		long double to_ldouble() const;
	private:
		string_t m_data;

#ifdef WIN32
		friend std::wostream& ::operator<<(std::wostream& out, const stdx::string& str);

		friend std::wistream& ::operator>>(std::wistream& in, stdx::string& str);
#else
		friend std::ostream& ::operator<<(std::ostream& out, const stdx::string& str);

		friend std::istream& ::operator>>(std::istream& in, stdx::string& str);
#endif
	};

	extern stdx::string to_string(int val);
	extern stdx::string to_string(long long int val);
	extern stdx::string to_string(double val);
	extern stdx::string to_string(long double val);
	extern stdx::string to_string(unsigned int val);
	extern stdx::string to_string(unsigned long long int val);
	extern stdx::string to_string(const typename stdx::string::char_t *str);
	extern const stdx::string &to_string(const stdx::string &val);
	extern stdx::string to_string(const std::string& val);
	extern stdx::string to_string(const typename stdx::string::char_t ch);
	extern stdx::string to_string(const char* str);
#ifdef WIN32
	extern stdx::string to_string(const std::wstring &val);
	extern stdx::string to_string(const char* str);
#endif
	template<typename ..._Args>
	void format_string(stdx::string& format_string, _Args&&...args)
	{
		_FormatString(format_string, std::move(std::initializer_list<stdx::string>{stdx::to_string(args)...}));
	}

	extern void _FormatString(stdx::string &format_string,std::initializer_list<stdx::string> &&args);

	//将两个HEX映射为字符
	extern char _MapByte(const std::pair<char, char>& pair);

	extern wchar_t _MapByte(const std::pair<wchar_t, wchar_t>& pair);
}

namespace std
{
	template<>
	class hash<stdx::string>
	{
	public:
		size_t operator()(const stdx::string &str) const
		{
			std::hash<typename stdx::string::string_t> hash;
			return hash(str.m_data);
		}
	};
}