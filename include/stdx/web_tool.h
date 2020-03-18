//#pragma once
//#include <ziran/encode_tool.h>
//#include <string>
//#include <memory>
//#include <boost/algorithm/string.hpp>
//
//namespace stdx
//{
//	namespace tools
//	{
//		namespace web
//		{
//			//����������ָ��MAP ������application/x-www-form-urlencoded
//			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body)
//			{
//				//����body�ĸ���
//				auto _body(body);
//				//�������vector
//				std::vector<std::string> res;
//				//��&�ָ�body
//				boost::algorithm::split(res, _body, boost::algorithm::is_any_of("&"));
//				//����MAP
//				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
//				try
//				{
//					//�����ָ���
//					for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
//					{
//						//��=�ָ� ���Խ����������ݲ���ȷ
//						std::vector<std::string> temp;
//						boost::algorithm::split(temp, *begin, boost::algorithm::is_any_of("="));
//						//���ö�Ӧ����
//						//�㶨
//						(*map_ptr)[url_decode(temp[0])] = url_decode(temp[1]);
//					}
//					return map_ptr;
//				}
//				catch (const std::out_of_range &)
//				{
//					throw std::invalid_argument("�Ƿ���Form Body:�޷���ȡ��ֵ��");
//				}
//			}
//			//����������ָ��MAP ������multipart/form-data
//			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body, const std::string &boundary)
//			{
//				//��boundary�ָ�body
//				std::vector<std::string> res = split_string(body, boundary);
//				//����MAP
//				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
//				//�����ָ���body
//				for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
//				{
//					//���Ϊ�վ�����
//					if (begin->empty())
//					{
//						continue;
//					}
//					//������CRLF�ָ��ͷ�������������
//					std::vector<std::string> temp = split_string(*begin, "\r\n\r\n");
//					try
//					{
//						//��ȡͷ��
//						std::string head = temp[0];
//						//��һ��CRLF���ָͬ��ͷ��
//						std::vector<std::string> headers = split_string(head, "\r\n");
//						//��ʾContent-Disposition
//						std::string disposition;
//						try
//						{
//							//������ͬ��ͷ��֪���ҵ�(���Ҳ���Content-Disposition)
//							for (auto _begin = std::begin(headers), _end = std::end(headers); _begin != _end; _begin++)
//							{
//								auto str = *_begin;
//								if (str.empty())
//								{
//									continue;
//								}
//								//�ָ��ַ��� ����Header��Value
//								auto header_and_value = split_string(str, ": ");
//								//����Խ�����˵�����ݲ���ȷ
//								if (header_and_value[0] == "Content-Disposition")
//								{
//									disposition = header_and_value[1];
//									break;
//								}
//							}
//						}
//						catch (const std::out_of_range &)
//						{
//							throw std::invalid_argument("�Ƿ���Form Body:�޷��ָ�Header");
//						}
//						//���Content-DispositionΪ�������ݲ���ȷ
//						if (disposition.empty())
//						{
//							throw std::invalid_argument("�Ƿ���Form Body:�޷��ҵ�Content-Disposition");
//						}
//						//��ʾname
//						//����Ҫ�����Ĺ�����(����)
//						std::string name;
//						//�ָ�Content-Disposition
//						auto disposition_item_vector = split_string(disposition, "; ");
//						try
//						{
//							//�����ָ���Content-Dispositionֱ���ҵ�name(�ǵģ�������������)
//							for (auto _begin = std::begin(disposition_item_vector), _end = std::end(disposition_item_vector); _begin != _end; _begin++)
//							{
//								//Ϊ��������
//								if (_begin->empty())
//								{
//									continue;
//								}
//								//��ȡֵ
//								auto disposition_str = *_begin;
//								//Ϊform-dataҲ����
//								if (disposition_str == "form-data")
//								{
//									continue;
//								}
//								//��=�ָ�Content-Disposition����
//								auto disposition_item = split_string(disposition_str, "=");
//								//�������Խ�����˵�����ݲ���ȷ
//								//�����name
//								if (disposition_item[0] == "name")
//								{
//									//��ǰname��ֵ
//									auto v_t = disposition_item[1];
//									//��Ҫǰ���"�ͺ����"
//									for (size_t i = 1, size = v_t.size(); i < (size - 1); i++)
//									{
//										//���push_back
//										name.push_back(v_t[i]);
//									}
//								}
//							}
//						}
//						catch (const std::out_of_range &)
//						{
//							throw std::invalid_argument("�Ƿ���Form Body:Content-Disposition����ȷ");
//						}
//						//name���Ϊ�������ݲ���ȷ
//						if (name.empty())
//						{
//							throw std::invalid_argument("�Ƿ���Form Body:Content-Disposition��name����Ϊ��");
//						}
//						(*map_ptr)[name] = temp[1];
//					}
//					catch (const std::out_of_range &)
//					{
//						throw std::invalid_argument("�Ƿ���Form Body:�޷��ҵ�\r\n\r\n(CRLF,CRLF)");
//					}
//				}
//				return map_ptr;
//			}
//			//��Content-Type��ȡboundary
//			std::string get_boundary(const std::string &content_type)
//			{
//				//��, �ָ��ַ�
//				auto vector = split_string(content_type, ", ");
//				//��������
//				for (auto begin = std::begin(vector), end = std::end(vector); begin != end; begin++)
//				{
//					//���Ϊmultipart/form-data������
//					if (*begin == "multipart/form-data")
//					{
//						continue;
//					}
//					//�ҵ�boundaryֱ�ӷ���
//					auto pair = split_to_half(*begin, "=");
//					if (pair.first == "boundary")
//					{
//						return pair.second;
//					}
//				}
//				//�Ҳ����׳��쳣
//				throw std::invalid_argument("��Ч�Ĳ���:�޷��ҵ�boundary");
//			}
//			//����������
//			enum form_type
//			{
//				application_x_www_form_urlencoded = 0, //application/x-www-form-urlencoded
//				multipart_form_data, //multipart/form-data
//				text_plain, //text/plain
//				application_json //application/json
//			};
//			//��ȡ����������
//			form_type get_form_type(const std::string &content_type)
//			{
//				if (content_type == "application/x-www-form-urlencoded")
//				{
//					return form_type::application_x_www_form_urlencoded;
//				}
//				if (content_type == "text/plain")
//				{
//					return form_type::text_plain;
//				}
//				if (content_type == "application/json")
//				{
//					return form_type::application_json;
//				}
//				if (content_type.find("multipart/form-data") != content_type.npos)
//				{
//					return form_type::multipart_form_data;
//				}
//				throw std::invalid_argument("δ֪��FormType");
//			}
//		}
//	}
//}