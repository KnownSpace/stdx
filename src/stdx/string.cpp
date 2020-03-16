#include <stdx/string.h>
#include <iostream>
#include <stdx/buffer.h>

stdx::string::string()
	:m_data()
{}

stdx::string::string(const char_t* str)
	:m_data(str)
{}

stdx::string::string(const string_t& str)
	:m_data(str)
{}

stdx::string::string(const stdx::string& other)
	:m_data(other.m_data)
{}

stdx::string::string(stdx::string&& other) noexcept
	:m_data(std::move(other.m_data))
{}

stdx::string &stdx::string::operator=(const stdx::string &other)
{
	m_data = other.m_data;
	return *this;
}

stdx::string &stdx::string::operator=(stdx::string &&other) noexcept
{
	m_data = std::move(other.m_data);
	return *this;
}

stdx::string& stdx::string::operator=(const stdx::string::char_t* str)
{
	m_data = str;
	return *this;
}

typename stdx::string::char_t& stdx::string::at(const size_type& pos)
{
	return m_data.at(pos);
}

typename stdx::string::char_t stdx::string::at(const size_type& pos) const
{
	return m_data.at(pos);
}

typename stdx::string::char_t& stdx::string::front()
{
	return m_data.front();
}

typename stdx::string::char_t stdx::string::front() const
{
	return m_data.front();
}

typename stdx::string::char_t& stdx::string::back()
{
	return m_data.back();
}

typename stdx::string::char_t stdx::string::back() const
{
	return m_data.back();
}

const stdx::string::char_t* stdx::string::data() const
{
	return m_data.data();
}

const stdx::string::char_t* stdx::string::c_str() const
{
	return m_data.c_str();
}

typename stdx::string::iterator_t stdx::string::begin()
{
	return m_data.begin();
}

typename stdx::string::const_iterator_t stdx::string::cbegin() const
{
	return m_data.cbegin();
}

typename stdx::string::iterator_t stdx::string::end()
{
	return m_data.end();
}

typename stdx::string::const_iterator_t stdx::string::cend() const
{
	return m_data.cend();
}

typename stdx::string::reverse_iterator_t stdx::string::rbegin()
{
	return m_data.rbegin();
}

typename stdx::string::const_reverse_iterator_t stdx::string::crbegin() const
{
	return m_data.crbegin();
}

typename stdx::string::reverse_iterator_t stdx::string::rend()
{
	return m_data.rend();
}

typename stdx::string::const_reverse_iterator_t stdx::string::crend() const
{
	return m_data.crend();
}

bool stdx::string::empty() const
{
	return m_data.empty();
}

typename stdx::string::size_type stdx::string::size() const
{
	return m_data.size();
}

typename stdx::string::size_type stdx::string::max_size() const
{
	return m_data.max_size();
}

typename stdx::string::size_type stdx::string::capacity() const
{
	return m_data.capacity();
}

void stdx::string::shrink_to_fit()
{
	m_data.shrink_to_fit();
}

void stdx::string::clear()
{
	m_data.clear();
}

void stdx::string::insert(const size_type& index, const char_t* str)
{
	m_data.insert(index, str);
}

void stdx::string::insert(const size_type& index, const char_t* str, const size_type& count)
{
	m_data.insert(index, str, count);
}

void stdx::string::erase(size_type index, size_type count)
{
	m_data.erase(index, count);
}

void stdx::string::erase(size_type index)
{
	m_data.erase(index);
}

typename stdx::string::iterator_t stdx::string::erase(iterator_t position)
{
	return m_data.erase(position);
}

typename stdx::string::iterator_t stdx::string::erase(iterator_t begin, iterator_t end)
{
	return m_data.erase(begin,end);
}

void stdx::string::push_back(const char_t& ch)
{
	m_data.push_back(ch);
}

void stdx::string::append(const char_t* str)
{
	m_data.append(str);
}

void stdx::string::append(const char_t* str, const size_type& size)
{
	m_data.append(str, size);
}

void stdx::string::append(const string& other)
{
	m_data.append(other.m_data);
}

bool stdx::string::begin_with(const char_t* str) const
{
#ifdef WIN32
	size_t size = wcslen(str);
#else
	size_t size = strlen(str);
#endif
	if (size > this->size())
	{
		return false;
	}
	for (size_t i = 0; i < size; i++)
	{
		if (str[i] != m_data.at(i))
		{
			return false;
		}
	}
	return true;
}

bool stdx::string::begin_with(const char_t &ch) const
{
	return ch == front();
}

bool stdx::string::begin_with(const string& other) const
{
	if (size() < other.size())
	{
		return false;
	}
	for (typename stdx::string::size_type i = 0,size = other.size(); i < size; i++)
	{
		if (at(i) != other.at(i))
		{
			return false;
		}
	}
	return true;
}

bool stdx::string::end_with(const char_t* str) const
{
#ifdef WIN32
	size_t size = wcslen(str);
#else
	size_t size = strlen(str);
#endif
	if (size > this->size())
	{
		return false;
	}
	typename stdx::string::size_type off = this->size() - size;
	for (size_t i = 0; i < size; i++)
	{
		if (m_data.at(off+i) != str[i])
		{
			return false;
		}
	}
	return true;
}

bool stdx::string::end_with(char_t ch) const
{
	return (back() == ch);
}

bool stdx::string::end_with(const string& other) const
{
	if (size() < other.size())
	{
		return false;
	}
	typename stdx::string::size_type off = this->size() - other.size();
	for (typename stdx::string::size_type i = 0, size = other.size(); i < size; i++)
	{
		if (at(i+off) != other.at(i))
		{
			return false;
		}
	}
	return true;
}

stdx::string &stdx::string::replace(const stdx::string& dest, const stdx::string& text)
{
	stdx::replace_string(m_data, dest.m_data, text.m_data);
	return *this;
}

stdx::string &stdx::string::replace(const size_type& pos, const size_type& count, const string& text)
{
	m_data.replace(pos, count, text.m_data);
	return *this;
}

stdx::string &stdx::string::replace(iterator_t begin,iterator_t end, const string& text)
{
	m_data.replace(begin, end, text.m_data);
	return *this;
}

stdx::string stdx::string::replace(const stdx::string& dest, const stdx::string& text) const
{
	stdx::string tmp(*this);
	tmp.replace(dest, text);
	return tmp;
}

stdx::string stdx::string::replace(const size_type& pos, const size_type& count, const string& text) const
{
	stdx::string tmp(*this);
	tmp.replace(pos, count, text.m_data);
	return tmp;
}

stdx::string stdx::string::replace(iterator_t begin, iterator_t end, const string& text) const
{
	stdx::string tmp(*this);
	tmp.replace(begin, end, text.m_data);
	return tmp;
}

stdx::string stdx::string::substr(size_type pos, size_type count) const
{
	return stdx::string(m_data.substr(pos,count));
}

void stdx::string::swap(string& other)
{
	m_data.swap(other.m_data);
}

typename stdx::string::size_type stdx::string::find(const string& str, size_type pos) const
{
	return m_data.find(str.m_data,pos);
}

typename stdx::string::size_type stdx::string::find(const char_t* s, size_type pos) const
{
	return m_data.find(s,pos);
}

typename stdx::string::size_type stdx::string::find(const char_t &ch, size_type pos) const
{
	return m_data.find(ch,pos);
}

typename stdx::string::size_type stdx::string::rfind(const string& str, size_type pos) const
{
	return m_data.rfind(str.m_data,pos);
}

typename stdx::string::size_type stdx::string::rfind(const char_t* s, size_type pos) const
{
	return m_data.rfind(s,pos);
}

typename stdx::string::size_type stdx::string::rfind(const char_t &ch, size_type pos) const
{
	return m_data.rfind(ch,pos);
}

typename stdx::string::size_type stdx::string::find_first_of(const string& str, size_type pos) const
{
	return m_data.find_first_of(str.m_data,pos);
}

typename stdx::string::size_type stdx::string::find_first_of(const char_t* s, size_type pos, size_type count) const
{
	return m_data.find_first_of(s,pos,count);
}

typename stdx::string::size_type stdx::string::find_first_of(const char_t* s, size_type pos) const
{
	return m_data.find_first_of(s,pos);
}

typename stdx::string::size_type stdx::string::find_first_of(const char_t &ch, size_type pos) const
{
	return m_data.find_first_of(ch,pos);
}

typename stdx::string::size_type stdx::string::find_last_of(const string& str, size_type pos) const
{
	return m_data.find_last_of(str.m_data,pos);
}

typename stdx::string::size_type stdx::string::find_last_of(const char_t* s, size_type pos, size_type count) const
{
	return m_data.find_last_of(s,pos,count);
}

typename stdx::string::size_type stdx::string::find_last_of(const char_t* s, size_type pos) const
{
	return m_data.find_last_of(s,pos);
}

typename stdx::string::size_type stdx::string::find_last_of(const char_t& ch, size_type pos) const
{
	return m_data.find_last_of(ch,pos);
}

bool stdx::string::equal(const string& other) const
{
	return (m_data == other.m_data);
}

bool stdx::string::equal(const char_t* str) const
{
	return (m_data == str);
}

typename stdx::string::string_t stdx::string::to_std_string() const
{
	return m_data;
}

std::string stdx::string::to_u8_string() const
{
#ifdef WIN32
	return stdx::unicode_to_utf8(m_data);
#else
	//Linux默认使用utf作为原生字符串
	return m_data;
#endif
}

std::string stdx::string::to_native_string() const
{
#ifdef WIN32
	return stdx::unicode_to_ansi(m_data);
#else
	return m_data;
#endif
}

stdx::string stdx::string::from_native_string(const std::string& str)
{
#ifdef WIN32
	return stdx::string(stdx::ansi_to_unicode(str));
#else
	return stdx::string(str);
#endif
}

stdx::string stdx::string::from_u8_string(const std::string& str)
{
#ifdef WIN32
	return stdx::string(stdx::utf8_to_unicode(str));
#else
	return stdx::string(str);
#endif
}

stdx::string stdx::string::from_buffer(const stdx::buffer &buf)
{
	stdx::string str;
#ifdef WIN32
	const wchar_t *p = (const wchar_t*)((const char*)buf);
	size_t size = buf.size();
	for(size_t i = 0,len = size/sizeof(wchar_t);i<len;++i)
	{
		str.push_back(p[i]);
	}
	//末尾处理
	if((size % 2) != 0)
	{
		int16_union u16;
		u16.low = buf[size-1];
		str.push_back((wchar_t)u16.value);
	}
#else
	for(size_t i = 0,size = buf.size();i<size;++i)
	{
		str.push_back(buf[i]);
	}
#endif
	return str;
}

void stdx::string::html_encode()
{
	this->replace(U("\""), U("&quot;"));
	this->replace(U("&"), U("&amp;"));
	this->replace(U(">"), U("&lt;"));
	this->replace(U("<"), U("&gt;"));
	this->replace(U("'"), U("&#39;"));
}

void stdx::string::html_decode()
{
	this->replace(U("&quot;"), U("\""));
	this->replace(U("&#34;"), U("\""));
	this->replace(U("&amp;"), U("&"));
	this->replace(U("&#38;"), U("&"));
	this->replace(U("&lt;"), U("<"));
	this->replace(U("&#60;"), U("<"));
	this->replace(U("&gt;"), U(">"));
	this->replace(U("&#62;"), U(">"));
	this->replace(U("&#39;"), U("'"));
}

#ifdef WIN32
std::wostream& operator<<(std::wostream& out, const stdx::string& str)
{
	return (out << str.m_data);
}

std::wistream& operator>>(std::wistream& in, stdx::string& str)
{
	return (in >> str.m_data);
}
#else
std::ostream& operator<<(std::ostream& out, const stdx::string& str)
{
	return (out << str.m_data);
}

std::istream& operator>>(std::istream& in, stdx::string& str)
{
	return (in >> str.m_data);
}
#endif

stdx::string stdx::to_string(int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

stdx::string stdx::to_string(long long int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

stdx::string stdx::to_string(double val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

stdx::string stdx::to_string(long double val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

stdx::string stdx::to_string(unsigned int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

stdx::string stdx::to_string(unsigned long long int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif
}

bool stdx::string::is_lower() const
{
	for (auto begin = m_data.begin(), end = m_data.end(); begin != end; begin++)
	{
#ifdef WIN32
		if (!iswlower(*begin))
		{
			return false;
		}
#else
		if (!islower(*begin))
		{
			return false;
		}
#endif
	}
	return true;
}

stdx::string &stdx::string::lower()
{
	for (auto begin=m_data.begin(),end=m_data.end();begin!=end;begin++)
	{
#ifdef WIN32
		if (iswupper(*begin))
		{
			*begin = towlower(*begin);
		}
#else
		if (isupper(*begin))
		{
			*begin = tolower(*begin);
		}
#endif
	}
	return *this;
}

stdx::string stdx::string::lower() const
{
	stdx::string tmp(*this);
	tmp.lower();
	return tmp;
}

bool stdx::string::is_upper() const
{
	for (auto begin = m_data.begin(),end = m_data.end();begin != end;begin++)
	{
#ifdef WIN32
		if (!iswupper(*begin))
		{
			return false;
		}
#else
		if (!isupper(*begin))
		{
			return false;
		}
#endif
	}
	return true;
}

stdx::string& stdx::string::upper()
{
	for (auto begin = m_data.begin(), end = m_data.end(); begin != end; begin++)
	{
#ifdef WIN32
		if (iswlower(*begin))
		{
			*begin = towupper(*begin);
		}
#else
		if (islower(*begin))
		{
			*begin = toupper(*begin);
		}
#endif
	}
	return *this;
}

stdx::string stdx::string::upper() const
{
	stdx::string tmp(*this);
	tmp.upper();
	return tmp;
}

std::string stdx::string::to_base64_string() const
{
	std::string str(to_native_string());
	return stdx::to_base64_string(str);
}

char stdx::switch_to_base64_char(unsigned char byte)
{
	if (byte == 62)
	{
		return '+';
	}
	else if(byte == 63)
	{
		return '-';
	}
	//A~Z
	if (byte <26)
	{
		return byte + 65;
	}
	//a~z
	if (byte>25 && byte <52)
	{
		return byte + 71;
	}
	//0~9
	if (byte > 51 && byte <64)
	{
		return byte - 4;
	}
	throw std::invalid_argument("unknow base64 index");
}

char stdx::base64_char_to_char(char ch)
{
	if (ch == '+')
	{
		return 62;
	}
	else if (ch == '-')
	{
		return 63;
	}
	if (ch >64  && ch < 91)
	{
		return ch - 65;
	}
	if (ch > 96 && ch<123)
	{
		return ch - 71;
	}
	if (ch >47 && ch < 58)
	{
		return ch+4;
	}
	throw std::invalid_argument("unknow base64 char");
}

stdx::string stdx::string::from_base64_string(const std::string& base64_str)
{
	return stdx::string::from_native_string(stdx::from_base64_string(base64_str));
}

std::string stdx::to_base64_string(const std::string& data)
{
	size_t size = data.size();
	size_t i = 0;
	std::string str;
	size_t end = size;
	size_t tmp = size % 3;
	if (tmp)
	{
		while (i != (end - tmp))
		{
			unsigned char t1 = data[i], t2 = data[i+1], t3 = data[i+2];
			str.push_back(stdx::switch_to_base64_char(t1 >> 2));
			t1 = (t1 & 0x03) << 4;
			str.push_back(stdx::switch_to_base64_char(t1 | (t2 >> 4)));
			t2 = (t2 & 0x0F) << 2;
			str.push_back(stdx::switch_to_base64_char((t2 | (t3 >> 6))));
			str.push_back(stdx::switch_to_base64_char(t3 & 0x3F));
			i += 3;
		}
		if (tmp == 1)
		{
			unsigned char t1 = data[i], t2 = (t1 & 0x03) << 4;
			t1 >>= 2;
			str.push_back(stdx::switch_to_base64_char(t1));
			str.push_back(stdx::switch_to_base64_char(t2));
			str.push_back('=');
			str.push_back('=');
		}
		else
		{
			unsigned char t1 = data[i], t2 = data[i+1], t3 = 0x00;
			str.push_back(stdx::switch_to_base64_char(t1 >> 2));
			t1 = (t1 & 0x03) << 4;
			str.push_back(stdx::switch_to_base64_char(t1 | (t2 >> 4)));
			t2 = (t2 & 0x0F) << 2;
			str.push_back(stdx::switch_to_base64_char((t2 | (t3 >> 6))));
			str.push_back('=');
		}
	}
	else
	{
		while (i != end)
		{
			unsigned char t1 = data[i], t2 = data[i+1], t3 = data[i+2];
			str.push_back(stdx::switch_to_base64_char(t1 >> 2));
			t1 = (t1 & 0x03) << 4;
			str.push_back(stdx::switch_to_base64_char(t1 | (t2 >> 4)));
			t2 = (t2 & 0x0F) << 2;
			str.push_back(stdx::switch_to_base64_char((t2 | (t3 >> 6))));
			str.push_back(stdx::switch_to_base64_char(t3 & 0x3F));
			i += 3;
		}
	}
	return str;
}

std::string stdx::from_base64_string(const std::string& base64_str)
{
	size_t size = base64_str.size();
	if (size % 4)
	{
		throw std::invalid_argument("invalid base64 string");
	}
	size_t i = 0;
	std::string str;
	while (i != size)
	{
		unsigned char t1 = base64_str[i],t2 = base64_str[i + 1],t3 = base64_str[i + 2],t4 = base64_str[i + 3];
		if (t1 != '=')
		{
			t1 = stdx::base64_char_to_char(t1);
		}
		else
		{
			return str;
		}
		if (t2 != '=')
		{
			t2 = stdx::base64_char_to_char(t2);
			t1 <<= 2;
			t1 |= (t2 >> 4);
			str.push_back(t1);
		}
		else
		{
			str.push_back(t1 << 2);
			return str;
		}
		if (t3 != '=')
		{
			t3 = stdx::base64_char_to_char(t3);
			t2 <<= 4;
			t2 |= (t3 >> 2);
			str.push_back(t2);
		}
		else
		{
			str.push_back(t2 << 4);
			return str;
		}
		if (t4 != '=')
		{
			t4 = stdx::base64_char_to_char(t4);
			t3 <<= 6;
			t3 |= t4;
			str.push_back(t3);
		}
		else
		{
			str.push_back(t3 << 6);
			return str;
		}
		i += 4;
	}
	return str;
}

stdx::string stdx::to_string(const typename stdx::string::char_t* str)
{
	return stdx::string(str);
}

const stdx::string& stdx::to_string(const stdx::string& val)
{
	return val;
}

stdx::string stdx::to_string(const std::string& val)
{
	return stdx::string::from_native_string(val);
}

#ifdef WIN32

stdx::string stdx::to_string(const std::wstring& val)
{
	return stdx::string(val);
}

stdx::string stdx::to_string(const char* str)
{
	std::string tmp(str);
	return stdx::string::from_native_string(tmp);
}

#endif

void stdx::string::erase(const stdx::string& target)
{
	for (size_t pos(m_data.find(target.m_data)); pos != npos; pos = m_data.find(target.m_data))
	{
		m_data.erase(pos, target.size());
	}
}

void stdx::string::erase(typename stdx::string::char_t ch)
{
	for (size_t pos(m_data.find(ch)); pos != npos; pos = m_data.find(ch))
	{
		m_data.erase(pos);
	}
}

void stdx::string::erase_once(const stdx::string& target)
{
	size_t pos(m_data.find(target.m_data));
	if (pos != npos)
	{
		m_data.erase(pos, target.size());
	}
}

void stdx::string::erase_once(typename stdx::string::char_t ch)
{
	size_t pos(m_data.find(ch));
	if (pos != npos)
	{
		m_data.erase(pos);
	}
}

void stdx::string::erase_front()
{
	erase(begin());
}

void stdx::string::erase_back()
{
	erase(end()-1);
}

stdx::string stdx::to_string(const typename stdx::string::char_t ch)
{
	stdx::string str;
	str.push_back(ch);
	return str;
}

int32_t stdx::string::to_int32() const
{
	return std::stoi(m_data);
}

int64_t stdx::string::to_int64() const
{
	return std::stoll(m_data);
}

uint32_t stdx::string::to_uint32() const
{
	return std::stoul(m_data);
}

uint64_t stdx::string::to_uint64() const
{
	return std::stoull(m_data);
}

double stdx::string::to_double() const
{
	return std::stod(m_data);
}

long double stdx::string::to_ldouble() const
{
	return std::stold(m_data);
}

void stdx::_FormatString(stdx::string& format_string, std::initializer_list<stdx::string>&& args)
{
	auto begin = args.begin();
	stdx::string str(U("{0}"));
	for (size_t i = 0, size = args.size(); i < size; i++)
	{
		if (i != 0)
		{
			str.replace(str.begin() + 1, str.end() - 1, stdx::to_string(i));
		}
		format_string.replace(str, *(begin + i));
	}
}

char stdx::_MapByte(const std::pair<char, char>& pair)
{
	//创建临时字符串
	std::string str;
	//将两个字符push_back
	str.push_back(pair.first);
	str.push_back(pair.second);
	//调用C标准库函数
	int i = std::stoi(str, nullptr, 16);
	//强制类型转换
	return (char)i;
}

wchar_t stdx::_MapByte(const std::pair<wchar_t, wchar_t>& pair)
{
	//创建临时字符串
	std::wstring str;
	//将两个字符push_back
	str.push_back(pair.first);
	str.push_back(pair.second);
	//调用C标准库函数
	int i = std::stoi(str, nullptr, 16);
	//强制类型转换
	return (wchar_t)i;
}

typename stdx::string::char_t stdx::string::hex_to_uchar(unsigned char byte)
{
	switch (byte)
	{
	case 0x00:
		return U('0');
	case 0x01:
		return U('1');
	case 0x02:
		return U('2');
	case 0x03:
		return U('3');
	case 0x04:
		return U('4');
	case 0x05:
		return U('5');
	case 0x06:
		return U('6');
	case 0x07:
		return U('7');
	case 0x08:
		return U('8');
	case 0x09:
		return U('9');
	case 0x0A:
		return U('A');
	case 0x0B:
		return U('B');
	case 0x0C:
		return U('C');
	case 0x0D:
		return U('D');
	case 0x0E:
		return U('E');
	case 0x0F:
		return U('F');
	default:
		throw std::domain_error("out of domain! should input 0x00 to 0x0F");
	}
}

char stdx::hex_to_char(unsigned char byte)
{
	switch (byte)
	{
	case 0x00:
		return '0';
	case 0x01:
		return '1';
	case 0x02:
		return '2';
	case 0x03:
		return '3';
	case 0x04:
		return '4';
	case 0x05:
		return '5';
	case 0x06:
		return '6';
	case 0x07:
		return '7';
	case 0x08:
		return '8';
	case 0x09:
		return '9';
	case 0x0A:
		return 'A';
	case 0x0B:
		return 'B';
	case 0x0C:
		return 'C';
	case 0x0D:
		return 'D';
	case 0x0E:
		return 'E';
	case 0x0F:
		return 'F';
	default:
		throw std::domain_error("out of domain! should input 0x00 to 0x0F");
	}
}

std::string stdx::url_decode(const std::string& str)
{
	std::string tmp(str);
	for (auto begin = tmp.begin(); begin != tmp.end(); begin++)
	{
		if (*begin == '%')
		{
			begin++;
			if (begin == tmp.end())
			{
				return tmp;
			}
			begin++;
			if (begin == tmp.end())
			{
				return tmp;
			}
			begin--;
			std::pair<char, char> pair;
			pair.first = 0;
			pair.second = 0;
			if (isdigit(*begin))
			{
				pair.first = *begin;
			}
			else if (isalpha(*begin))
			{
				if (isupper(*begin))
				{
					*begin = tolower(*begin);
				}
				switch (*begin)
				{
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					pair.first = *begin;
					break;
				default:
					continue;
				}
			}
			else
			{
				continue;
			}
			begin++;
			if (isdigit(*begin))
			{
				pair.second = *begin;
			}
			else if (isalpha(*begin))
			{
				if (isupper(*begin))
				{
					*begin = tolower(*begin);
				}
				switch (*begin)
				{
				case U('a'):
				case U('b'):
				case U('c'):
				case U('d'):
				case U('e'):
				case U('f'):
					pair.second = *begin;
					break;
				default:
					pair.first = 0;
					continue;
				}
			}
			else
			{
				continue;
			}
			if (pair.first != 0 && pair.second != 0)
			{
				auto bound = begin + 1;
				begin--;
				begin--;
				char ch = stdx::_MapByte(pair);
				begin = tmp.erase(begin, bound);
				begin = tmp.insert(begin, ch);
			}
		}
	}
	return tmp;
}

std::string stdx::url_encode(const std::string& str)
{
	std::string tmp(str);
	for (auto begin = tmp.begin();begin!=tmp.end();begin++)
	{
		if (!isalnum(*begin))
		{
			unsigned char height = *begin &0xF0;
			height >>= 4;
			unsigned char low = *begin & 0x0F;
			begin = tmp.erase(begin);
			begin = tmp.insert(begin, '%');
			begin++;
			begin = tmp.insert(begin, stdx::hex_to_char(height));
			begin++;
			begin = tmp.insert(begin, stdx::hex_to_char(low));
		}
	}
	return tmp;
}

void stdx::string::url_encode()
{
#ifdef WIN32
	std::string tmp(to_native_string());
	this->operator=(std::move(stdx::string::from_native_string(stdx::url_encode(tmp))));
#else
	this->operator=(std::move(stdx::string::from_native_string(stdx::url_encode(to_native_string()))));
#endif
}

void stdx::string::url_decode()
{
#ifdef WIN32
	std::string tmp(to_native_string());
	this->operator=(std::move(stdx::string::from_native_string(stdx::url_decode(tmp))));
#else
	this->operator=(std::move(stdx::string::from_native_string(stdx::url_decode(to_native_string()))));
#endif
}