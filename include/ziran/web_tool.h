#pragma once
#include "code_tool.h"
#include <string>
#include <memory>
#include <boost/algorithm/string.hpp>

namespace ziran
{
	namespace tools
	{
		namespace web
		{
			//�������ַ�ӳ��Ϊ�ֽ�
			char map_byte(const std::pair<char, char> &pair)
			{
				//������ʱ�ַ���
				std::string str;
				//�������ַ�push_back
				str.push_back(pair.first);
				str.push_back(pair.second);
				//����ָʾend��ָ��
				char *end;
				//����C��׼�⺯��
				int i = strtol(str.c_str(), &end, 16);
				//ǿ������ת��
				return (char)i;
			}
			//URL����
			std::string url_decode(const std::string &args)
			{
				//����������
				std::vector<char> buffer;
				//��ȡbegin,end������
				auto begin = std::begin(args), end = std::end(args);
				//��ʼ����
				while (begin != end)
				{
					//����ַ��Ƿ�Ϊ%
					if (*begin != '%')
					{
						//������ƫ�Ƶ�������push_back��������
						begin++;
						buffer.push_back(*begin);
						//��������
						continue;
					}
					//�����%
					//��ʼ��ԭת��
					//ƫ�Ƶ���һλ
					begin++;
					//���������endλ��
					if (begin != end)
					{
						//����pair
						std::pair<char, char> pair;
						//��first����Ϊ��һ���ַ�
						pair.first = *begin;
						//ƫ�Ƶ�����
						begin++;
						//���������end
						if (begin != end)
						{
							//��second����Ϊ�ڶ����ַ�
							pair.second = *begin;
							//ƫ�Ƶ�����
							begin++;
						}
						//�������end ˵����ʽ����ȷ
						else
						{
							//��second����Ϊ��һ���ַ�
							pair.second = pair.first;
							//��first����Ϊ'0'
							pair.first = '0';
						}
						//�������ַ�ӳ��Ϊbyte��push_back
						buffer.push_back(map_byte(pair));
					}
					//�������end ˵����ʽ����ȷ
					//ʹ�ò��ȴ�ʩ�����н���
					else
					{
						buffer.push_back(*(begin--));
						begin++;
					}
				}
				//ʹ�û���������string
				return std::string(buffer.begin(), buffer.end());
			}
			//����������ָ��MAP ������application/x-www-form-urlencoded
			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body)
			{
				//����body�ĸ���
				auto _body(body);
				//�������vector
				std::vector<std::string> res;
				//��&�ָ�body
				boost::algorithm::split(res, _body, boost::algorithm::is_any_of("&"));
				//����MAP
				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
				try
				{
					//�����ָ���
					for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
					{
						//��=�ָ� ���Խ����������ݲ���ȷ
						std::vector<std::string> temp;
						boost::algorithm::split(temp, *begin, boost::algorithm::is_any_of("="));
						//���ö�Ӧ����
						//�㶨
						(*map_ptr)[url_decode(temp[0])] = url_decode(temp[1]);
					}
					return map_ptr;
				}
				catch (const std::out_of_range &)
				{
					throw std::invalid_argument("�Ƿ���Form Body:�޷���ȡ��ֵ��");
				}
			}
			//���ַ����ָ����������
			std::pair<std::string, std::string> split_to_half(const std::string &str, const std::string &pattern)
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
			std::vector<std::string> split_string(const std::string &str, const std::string &pattern)
			{
				//����vector������
				std::vector<std::string> result;
				//�ȷָ��һ��
				auto pair = split_to_half(str, pattern);
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
			//����������ָ��MAP ������multipart/form-data
			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body, const std::string &boundary)
			{
				//��boundary�ָ�body
				std::vector<std::string> res = split_string(body, boundary);
				//����MAP
				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
				//�����ָ���body
				for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
				{
					//���Ϊ�վ�����
					if (begin->empty())
					{
						continue;
					}
					//������CRLF�ָ��ͷ�������������(��?Ӧ����ô��)
					std::vector<std::string> temp = split_string(*begin, "\r\n\r\n");
					try
					{
						//��ȡͷ��
						std::string head = temp[0];
						//��һ��CRLF���ָͬ��ͷ��
						std::vector<std::string> headers = split_string(head, "\r\n");
						//��ʾContent-Disposition
						std::string disposition;
						try
						{
							//������ͬ��ͷ��֪���ҵ�(���Ҳ���Content-Disposition)
							for (auto _begin = std::begin(headers), _end = std::end(headers); _begin != _end; _begin++)
							{
								auto str = *_begin;
								if (str.empty())
								{
									continue;
								}
								//�ָ��ַ��� ����Header��Value
								auto header_and_value = split_string(str, ": ");
								//����Խ�����˵�����ݲ���ȷ
								if (header_and_value[0] == "Content-Disposition")
								{
									disposition = header_and_value[1];
									break;
								}
							}
						}
						catch (const std::out_of_range &)
						{
							throw std::invalid_argument("�Ƿ���Form Body:�޷��ָ�Header");
						}
						//���Content-DispositionΪ�������ݲ���ȷ
						if (disposition.empty())
						{
							throw std::invalid_argument("�Ƿ���Form Body:�޷��ҵ�Content-Disposition");
						}
						//��ʾname
						//����Ҫ�����Ĺ�����(����)
						std::string name;
						//�ָ�Content-Disposition
						auto disposition_item_vector = split_string(disposition, "; ");
						try
						{
							//�����ָ���Content-Dispositionֱ���ҵ�name(�ǵģ�������������)
							for (auto _begin = std::begin(disposition_item_vector), _end = std::end(disposition_item_vector); _begin != _end; _begin++)
							{
								//Ϊ��������
								if (_begin->empty())
								{
									continue;
								}
								//��ȡֵ
								auto disposition_str = *_begin;
								//Ϊform-dataҲ����
								if (disposition_str == "form-data")
								{
									continue;
								}
								//��=�ָ�Content-Disposition����
								auto disposition_item = split_string(disposition_str, "=");
								//�������Խ�����˵�����ݲ���ȷ
								//�����name
								if (disposition_item[0] == "name")
								{
									//��ǰname��ֵ
									auto v_t = disposition_item[1];
									//��Ҫǰ���"�ͺ����"
									for (size_t i = 1, size = v_t.size(); i < (size - 1); i++)
									{
										//���push_back
										name.push_back(v_t[i]);
									}
								}
							}
						}
						catch (const std::out_of_range &)
						{
							throw std::invalid_argument("�Ƿ���Form Body:Content-Disposition����ȷ");
						}
						//name���Ϊ�������ݲ���ȷ
						if (name.empty())
						{
							throw std::invalid_argument("�Ƿ���Form Body:Content-Disposition��name����Ϊ��");
						}
						//����MAP,������һ��ѭ��������
						(*map_ptr)[name] = temp[1];
					}
					catch (const std::out_of_range &)
					{
						throw std::invalid_argument("�Ƿ���Form Body:�޷��ҵ�\r\n\r\n(CRLF,CRLF)");
					}
				}
				return map_ptr;
			}
			//��Content-Type��ȡboundary
			std::string get_boundary(const std::string &content_type)
			{
				//��, �ָ��ַ�
				auto vector = split_string(content_type, ", ");
				//��������
				for (auto begin = std::begin(vector), end = std::end(vector); begin != end; begin++)
				{
					//���Ϊmultipart/form-data������
					if (*begin == "multipart/form-data")
					{
						continue;
					}
					//�ҵ�boundaryֱ�ӷ���
					auto pair = split_to_half(*begin, "=");
					if (pair.first == "boundary")
					{
						return pair.second;
					}
				}
				//�Ҳ����׳��쳣
				throw std::invalid_argument("��Ч�Ĳ���:�޷��ҵ�boundary");
			}
			//����������
			enum form_type
			{
				application_x_www_form_urlencoded = 0, //application/x-www-form-urlencoded
				multipart_form_data, //multipart/form-data
				text_plain, //text/plain
				application_json //application/json
			};
			//��ȡ����������
			form_type get_form_type(const std::string &content_type)
			{
				if (content_type == "application/x-www-form-urlencoded")
				{
					return form_type::application_x_www_form_urlencoded;
				}
				if (content_type == "text/plain")
				{
					return form_type::text_plain;
				}
				if (content_type == "application/json")
				{
					return form_type::application_json;
				}
				if (content_type.find("multipart/form-data") != content_type.npos)
				{
					return form_type::multipart_form_data;
				}
				throw std::invalid_argument("δ֪��FormType");
			}
		}
	}
}