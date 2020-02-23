#pragma once
#include <stdx/env.h>
#include <vector>
#include <stdx/string.h>

namespace stdx
{
	struct string;

	enum class big_int_symbol
	{
		zero,		//0
		positive,	//正数
		negative	//负数
	};
	struct _BigInt
	{
		using byte_t = unsigned char;
	public:
		_BigInt();

		_BigInt(uint8_t v);

		_BigInt(int8_t v);

		_BigInt(uint16_t v);

		_BigInt(int16_t v);

		_BigInt(uint32_t v);

		_BigInt(int32_t v);

		_BigInt(uint64_t v);

		_BigInt(int64_t v);

		_BigInt(byte_t* buffer, const size_t& size);

		_BigInt(const _BigInt& other);

		_BigInt(_BigInt &&other) noexcept;

		~_BigInt()=default;

		//1 >
		//0 ==
		//-1 <
		int _BitCompareWith(byte_t* buffer, const size_t& buffer_size) const;

		int _BitCompareWith(const std::vector<byte_t> &other) const;

		static stdx::uint16_union _BitAdd(byte_t a, byte_t b);

		void _BitAdd(byte_t* buffer, const size_t& buffer_size);

		void _BitAdd(const std::vector<byte_t>& other);

		static void _BitBack(byte_t *buffer,const size_t &buffer_size);

		static void _BitBack(byte_t *symbol,byte_t *buf,const size_t &buf_size);

		static void _BitBack(byte_t* symbol,std::vector<byte_t> &other);

		static void _BitAddOne(byte_t *symbol,byte_t* buffer,const size_t &buffer_size);

		static void _BitAddOne(byte_t* buffer, const size_t& buffer_size);

		static void _BitAddOne(byte_t* symbol, std::vector<byte_t> &other);

		static void _ToComplement(byte_t* buffer, const size_t& buffer_size);

		static void _ToComplement(byte_t *symbol,byte_t *buffer,const size_t &buffer_size);

		static void _ToComplement(byte_t* symbol, std::vector<byte_t> &other);

		static void _ToTureForm(byte_t *buffer,const size_t &buffer_size);

		static void _ToTureForm(byte_t *symbol,byte_t* buffer, const size_t& buffer_size);

		static void _ToTureForm(byte_t* symbol, std::vector<byte_t> &other);

		bool _BitSubstract(byte_t* buffer, const size_t& buffer_size);

		bool _BitSubstract(const std::vector<byte_t>& other);

		void _BitOr(byte_t* buffer, const size_t& buffer_size);

		void _BitOr(const std::vector<byte_t>& other);

		void _BitXor(byte_t* buffer, const size_t& buffer_size);

		void _BitXor(const std::vector<byte_t>& other);

		void _BitAnd(byte_t* buffer, const size_t& buffer_size);

		void _BitAnd(const std::vector<byte_t>& other);

		static typename stdx::string::char_t _ByteToUChar(byte_t v);

		static byte_t _UCharToByte(const typename stdx::string::char_t &ch);

		static stdx::_BigInt from_hex_string(const stdx::string &hex);

		static stdx::_BigInt from_hex_string(stdx::string &&hex);

		template<typename _IntType>
		static void _RemoveSymbol(_IntType& v)
		{
			if (v < 0)
			{
				v = (-v);
			}
		}

		bool operator==(int8_t v) const;

		bool operator==(int16_t v) const;

		bool operator==(int32_t v) const;

		bool operator==(int64_t v) const;

		bool operator==(uint8_t v) const;

		bool operator==(uint16_t v) const;

		bool operator==(uint32_t v) const;

		bool operator==(uint64_t v) const;

		bool operator==(const _BigInt& other) const;

		bool operator!=(int8_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(int16_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(int32_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(int64_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(uint8_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(uint16_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(uint32_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(uint64_t v) const
		{
			return !this->operator==(v);
		}

		bool operator!=(const _BigInt &other) const
		{
			return !this->operator==(other);
		}

		stdx::_BigInt& operator=(int8_t v);

		stdx::_BigInt& operator=(int16_t v);

		stdx::_BigInt& operator=(int32_t v);

		stdx::_BigInt& operator=(int64_t v);

		stdx::_BigInt& operator=(uint8_t v);

		stdx::_BigInt& operator=(uint16_t v);

		stdx::_BigInt& operator=(uint32_t v);

		stdx::_BigInt& operator=(uint64_t v);

		stdx::_BigInt& operator=(const _BigInt& other);

		stdx::_BigInt& operator=(_BigInt &&other) noexcept;

		bool operator>(int8_t v) const;

		bool operator>(int16_t v) const;

		bool operator>(int32_t v) const;

		bool operator>(int64_t v) const;

		bool operator>(uint8_t v) const;

		bool operator>(uint16_t v) const;

		bool operator>(uint32_t v) const;

		bool operator>(uint64_t v) const;

		bool operator>(const _BigInt& other)const;

		bool operator<(int8_t v) const;

		bool operator<(int16_t v) const;

		bool operator<(int32_t v) const;

		bool operator<(int64_t v) const;

		bool operator<(uint8_t v) const;

		bool operator<(uint16_t v) const;

		bool operator<(uint32_t v) const;

		bool operator<(uint64_t v) const;

		bool operator<(const _BigInt &other) const;

		bool operator>=(int8_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(int16_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(int32_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(int64_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(uint8_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(uint16_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(uint32_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(uint64_t v) const
		{
			return !this->operator<(v);
		}

		bool operator>=(const _BigInt& other)const
		{
			return !this->operator<(other);
		}

		bool operator<=(int8_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(int16_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(int32_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(int64_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(uint8_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(uint16_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(uint32_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(uint64_t v) const
		{
			return !this->operator>(v);
		}

		bool operator<=(const _BigInt& other) const
		{
			return !this->operator>(other);
		}

		void operator+=(int8_t v);

		void operator+=(int16_t v);

		void operator+=(int32_t v);

		void operator+=(int64_t v);

		void operator+=(uint8_t v);

		void operator+=(uint16_t v);

		void operator+=(uint32_t v);

		void operator+=(uint64_t v);

		void operator+=(const _BigInt &other);

		void operator-=(int8_t v);

		void operator-=(int16_t v);

		void operator-=(int32_t v);

		void operator-=(int64_t v);

		void operator-=(uint8_t v);

		void operator-=(uint16_t v);

		void operator-=(uint32_t v);

		void operator-=(uint64_t v);

		void operator-=(const _BigInt &other);

		stdx::_BigInt operator+(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp += v;
			return tmp;
		}

		stdx::_BigInt operator+(const _BigInt &other) const
		{
			stdx::_BigInt tmp(*this);
			tmp += other;
			return tmp;
		}

		stdx::_BigInt operator-(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= v;
			return tmp;
		}

		stdx::_BigInt operator-(const _BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp -= other;
			return tmp;
		}

		void operator++()
		{
			this->operator+=(1);
		}

		void operator++(int)
		{
			this->operator+=(1);
		}

		void operator--()
		{
			this->operator-=(1);
		}

		void operator--(int)
		{
			this->operator-=(1);
		}

		stdx::_BigInt operator-() const
		{
			if (m_symbol == stdx::big_int_symbol::zero)
			{
				return *this;
			}
			stdx::_BigInt tmp(*this);
			tmp.m_symbol = (tmp.m_symbol == stdx::big_int_symbol::positive) ? stdx::big_int_symbol::negative : stdx::big_int_symbol::positive;
			return tmp;
		}

		stdx::_BigInt operator+() const
		{
			return *this;
		}

		void operator*=(int8_t v);

		void operator*=(int16_t v);

		void operator*=(int32_t v);

		void operator*=(int64_t v);

		void operator*=(uint8_t v);

		void operator*=(uint16_t v);

		void operator*=(uint32_t v);

		void operator*=(uint64_t v);

		void operator*=(const _BigInt &other);

		stdx::_BigInt operator*(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= v;
			return tmp;
		}

		stdx::_BigInt operator*(const _BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp *= other;
			return tmp;
		}

		void operator/=(int8_t v);

		void operator/=(int16_t v);

		void operator/=(int32_t v);

		void operator/=(int64_t v);

		void operator/=(uint8_t v);

		void operator/=(uint16_t v);

		void operator/=(uint32_t v);

		void operator/=(uint64_t v);

		void operator/=(const _BigInt& other);

		stdx::_BigInt operator/(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= v;
			return tmp;
		}

		stdx::_BigInt operator/(const _BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp /= other;
			return tmp;
		}

		void operator%=(int8_t v);

		void operator%=(int16_t v);

		void operator%=(int32_t v);

		void operator%=(int64_t v);

		void operator%=(uint8_t v);

		void operator%=(uint16_t v);

		void operator%=(uint32_t v);

		void operator%=(uint64_t v);

		void operator%=(const _BigInt& other);

		stdx::_BigInt operator%(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= v;
			return tmp;
		}

		stdx::_BigInt operator%(const stdx::_BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp %= other;
			return tmp;
		}

		void operator<<=(const size_t &n);

		stdx::_BigInt operator<<(const size_t& n) const
		{
			stdx::_BigInt tmp(*this);
			tmp <<= n;
			return tmp;
		}

		void operator>>=(const size_t& n);

		stdx::_BigInt operator>>(const size_t& n) const
		{
			stdx::_BigInt tmp(*this);
			tmp >>= n;
			return tmp;
		}

		void operator|=(int8_t v);

		void operator|=(int16_t v);

		void operator|=(int32_t v);

		void operator|=(int64_t v);

		void operator|=(uint8_t v);

		void operator|=(uint16_t v);

		void operator|=(uint32_t v);

		void operator|=(uint64_t v);

		void operator|=(const stdx::_BigInt &other);

		stdx::_BigInt operator|(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= v;
			return tmp;
		}

		stdx::_BigInt operator|(const stdx::_BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp |= other;
			return tmp;
		}

		void operator^=(int8_t v);

		void operator^=(int16_t v);

		void operator^=(int32_t v);

		void operator^=(int64_t v);

		void operator^=(uint8_t v);

		void operator^=(uint16_t v);

		void operator^=(uint32_t v);

		void operator^=(uint64_t v);

		void operator^=(const stdx::_BigInt &other);

		stdx::_BigInt operator^(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= v;
			return tmp;
		}

		stdx::_BigInt operator^(const stdx::_BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp ^= other;
			return tmp;
		}

		void operator&=(int8_t v);

		void operator&=(int16_t v);

		void operator&=(int32_t v);

		void operator&=(int64_t v);

		void operator&=(uint8_t v);

		void operator&=(uint16_t v);

		void operator&=(uint32_t v);

		void operator&=(uint64_t v);

		void operator&=(const stdx::_BigInt &other);

		stdx::_BigInt operator&(int8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(int16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(int32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(int64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(uint8_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(uint16_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(uint32_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(uint64_t v) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= v;
			return tmp;
		}

		stdx::_BigInt operator&(const stdx::_BigInt& other) const
		{
			stdx::_BigInt tmp(*this);
			tmp &= other;
			return tmp;
		}
		
		stdx::string to_hex_string() const;

		stdx::string to_hex_string_without_0x() const;
	private:
		stdx::big_int_symbol m_symbol;
		std::vector<byte_t> m_data;
	};
}