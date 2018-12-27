#pragma once
#include <vector>

namespace stdx
{
	struct sort_way
	{
		enum
		{
			bigger = 0,
			smaller = 1
		};
	};
	//�������򷽷�(�Ӵ�С)
	template<typename _T, typename _TContainer = std::vector<_T>>
	void quicksort_bigger(_TContainer &container, size_t begin, size_t end)
	{
		if (end == 0)
		{
			return;
		}
		end = end - 1;
		if (begin > end)
		{
			return;
		}
		auto basic = container[begin];
		auto begin_ps = begin;
		auto end_ps = end;
		while (begin != end)
		{
			while (container[end] <= basic && begin < end)
			{
				end--;
			}
			while (container[begin] >= basic && begin < end)
			{
				begin++;
			}
			if (begin<end)
			{
				auto temp = container[begin];
				container[begin] = container[end];
				container[end] = temp;
			}
		}
		container[begin_ps] = container[begin];
		container[begin] = basic;
		quicksort_bigger<_T, _TContainer>(container, begin_ps, begin);
		quicksort_bigger<_T, _TContainer>(container, begin + 1, end_ps + 1);
	}
	//�������򷽷�(��С����)
	template<typename T, typename TContainer = std::vector<T>>
	void quicksort_smaller(TContainer &container, size_t begin, size_t end)
	{
		if (end == 0)
		{
			return;
		}
		end = end - 1;
		if (begin > end)
		{
			return;
		}
		auto basic = container[begin];
		auto begin_ps = begin;
		auto end_ps = end;
		while (begin != end)
		{
			while (container[end] >= basic && begin < end)
			{
				end--;
			}
			while (container[begin] <= basic && begin < end)
			{
				begin++;
			}
			if (begin<end)
			{
				auto temp = container[begin];
				container[begin] = container[end];
				container[end] = temp;
			}
		}
		container[begin_ps] = container[begin];
		container[begin] = basic;
		quicksort_smaller<T, TContainer>(container, begin_ps, begin);
		quicksort_smaller<T, TContainer>(container, begin + 1, end_ps + 1);
	}

	//���ַ����ָ����������
	template<typename _TString = std::string>
	std::pair<_TString, _TString> split_to_double(const _TString &str, const _TString &pattern)
	{
		//����ǿ��򱨴�
		if (pattern.empty())
		{
			throw std::invalid_argument("���� pattern ����Ϊ���ַ���,�޷�����pattern�ָ�str");
		}
		//����һ��pair����������
		auto pair = std::make_pair<std::string, std::string>("", "");
		//ʹ��STL�ָ�
		//���ҵ�Ҫ���ָ��ַ�����λ��
		size_t pos = str.find(pattern);
		//����Ҳ���
		if (pos == str.npos)
		{
			//�ж�str�Ƿ�Ϊ��
			if (!str.empty())
			{
				//�������˵��û������ַ���
				//��ȫ�����õ�first��
				pair.first = str;
			}
		}
		//�ҵ���
		else
		{
			//����first��
			pair.first = str.substr(0, pos);
			//����second��,ע��ƫ����
			pair.second = str.substr(pos + pattern.size(), str.size());
		}
		return pair;
	}

	//���ַ����ָ�ɼ�������
	template<typename _TString = std::string>
	std::vector<_TString> split_string(const _TString &str, const _TString &pattern)
	{
		//����vector������
		std::vector<_TString> result;
		//�ȷָ��һ��
		auto pair = split_to_half<_TString>(str, pattern);
		//��first push_back
		result.push_back(pair.first);
		//�鿴�Ƿ���Ҫ�����ָ�
		while (!pair.second.empty())
		{
			//�����ָ�һ��
			pair = split_to_half(pair.second, pattern);
			//��first push_back
			result.push_back(pair.first);
		}
		//���ؽ��
		return result;
	}
}