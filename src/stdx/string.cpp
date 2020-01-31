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

//typename stdx::string::iterator_t stdx::string::erase(const_iterator_t position)
//{
//	return m_data.erase(position);
//}

typename stdx::string::iterator_t stdx::string::erase(iterator_t begin, iterator_t end)
{
	return m_data.erase(begin,end);
}

//typename stdx::string::iterator_t stdx::string::erase(const_iterator_t begin, const_iterator_t end)
//{
//	return m_data.erase(begin,end);
//}

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

void stdx::string::replace(const stdx::string& dest, const stdx::string& text)
{
	stdx::replace_string(m_data, dest.m_data, text.m_data);
}

void stdx::string::replace(const size_type& pos, const size_type& count, const string& text)
{
	m_data.replace(pos, count, text.m_data);
}

void stdx::string::replace(iterator_t begin,iterator_t end, const string& text)
{
	m_data.replace(begin, end, text.m_data);
}

stdx::string stdx::string::substr(size_type pos, size_type count) const
{
	return string(m_data.substr(pos,count));
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
	int16_union u;
	for(size_t i = 0,size = buf.size();i<size;++i)
	{
		u.low = buf[i];
		str.push_back((wchar_t)u.value);
	}
#else
	for(size_t i = 0,size = buf.size();i<size;++i)
	{
		str.push_back(buf[i]);
	}
#endif
	return str;
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
#endif // WIN32
}

stdx::string stdx::to_string(long long int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif // WIN32
}

stdx::string stdx::to_string(double val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif // WIN32
}

stdx::string stdx::to_string(long double val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif // WIN32
}

stdx::string stdx::to_string(unsigned int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif // WIN32
}

stdx::string stdx::to_string(unsigned long long int val)
{
#ifdef WIN32
	return std::to_wstring(val);
#else
	return std::to_string(val);
#endif // WIN32
}
