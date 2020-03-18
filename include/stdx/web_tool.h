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
//			//将请求主体分割成MAP 适用于application/x-www-form-urlencoded
//			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body)
//			{
//				//创建body的副本
//				auto _body(body);
//				//创建结果vector
//				std::vector<std::string> res;
//				//用&分割body
//				boost::algorithm::split(res, _body, boost::algorithm::is_any_of("&"));
//				//创建MAP
//				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
//				try
//				{
//					//遍历分割结果
//					for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
//					{
//						//用=分割 如果越界访问则数据不正确
//						std::vector<std::string> temp;
//						boost::algorithm::split(temp, *begin, boost::algorithm::is_any_of("="));
//						//设置对应的项
//						//搞定
//						(*map_ptr)[url_decode(temp[0])] = url_decode(temp[1]);
//					}
//					return map_ptr;
//				}
//				catch (const std::out_of_range &)
//				{
//					throw std::invalid_argument("非法的Form Body:无法获取键值对");
//				}
//			}
//			//将请求主体分割成MAP 适用于multipart/form-data
//			std::shared_ptr<std::map<std::string, std::string>> split_request_body(const std::string &body, const std::string &boundary)
//			{
//				//按boundary分割body
//				std::vector<std::string> res = split_string(body, boundary);
//				//创建MAP
//				std::shared_ptr<std::map<std::string, std::string>> map_ptr = std::make_shared<std::map<std::string, std::string>>();
//				//遍历分割后的body
//				for (auto begin = std::begin(res), end = std::end(res); begin != end; begin++)
//				{
//					//如果为空就跳过
//					if (begin->empty())
//					{
//						continue;
//					}
//					//已两个CRLF分割成头部和主体的主体
//					std::vector<std::string> temp = split_string(*begin, "\r\n\r\n");
//					try
//					{
//						//获取头部
//						std::string head = temp[0];
//						//用一个CRLF来分割不同的头部
//						std::vector<std::string> headers = split_string(head, "\r\n");
//						//表示Content-Disposition
//						std::string disposition;
//						try
//						{
//							//遍历不同的头部知道找到(或找不到Content-Disposition)
//							for (auto _begin = std::begin(headers), _end = std::end(headers); _begin != _end; _begin++)
//							{
//								auto str = *_begin;
//								if (str.empty())
//								{
//									continue;
//								}
//								//分割字符串 区别Header和Value
//								auto header_and_value = split_string(str, ": ");
//								//这里越界访问说明数据不正确
//								if (header_and_value[0] == "Content-Disposition")
//								{
//									disposition = header_and_value[1];
//									break;
//								}
//							}
//						}
//						catch (const std::out_of_range &)
//						{
//							throw std::invalid_argument("非法的Form Body:无法分割Header");
//						}
//						//如果Content-Disposition为空则数据不正确
//						if (disposition.empty())
//						{
//							throw std::invalid_argument("非法的Form Body:无法找到Content-Disposition");
//						}
//						//表示name
//						//终于要做最后的工作了(激动)
//						std::string name;
//						//分割Content-Disposition
//						auto disposition_item_vector = split_string(disposition, "; ");
//						try
//						{
//							//遍历分割后的Content-Disposition直到找到name(是的，放弃其他数据)
//							for (auto _begin = std::begin(disposition_item_vector), _end = std::end(disposition_item_vector); _begin != _end; _begin++)
//							{
//								//为空则跳过
//								if (_begin->empty())
//								{
//									continue;
//								}
//								//获取值
//								auto disposition_str = *_begin;
//								//为form-data也跳过
//								if (disposition_str == "form-data")
//								{
//									continue;
//								}
//								//用=分割Content-Disposition的项
//								auto disposition_item = split_string(disposition_str, "=");
//								//这里出现越界访问说明数据不正确
//								//如果是name
//								if (disposition_item[0] == "name")
//								{
//									//提前name的值
//									auto v_t = disposition_item[1];
//									//不要前面的"和后面的"
//									for (size_t i = 1, size = v_t.size(); i < (size - 1); i++)
//									{
//										//逐个push_back
//										name.push_back(v_t[i]);
//									}
//								}
//							}
//						}
//						catch (const std::out_of_range &)
//						{
//							throw std::invalid_argument("非法的Form Body:Content-Disposition不正确");
//						}
//						//name如果为空则数据不正确
//						if (name.empty())
//						{
//							throw std::invalid_argument("非法的Form Body:Content-Disposition的name属性为空");
//						}
//						(*map_ptr)[name] = temp[1];
//					}
//					catch (const std::out_of_range &)
//					{
//						throw std::invalid_argument("非法的Form Body:无法找到\r\n\r\n(CRLF,CRLF)");
//					}
//				}
//				return map_ptr;
//			}
//			//从Content-Type获取boundary
//			std::string get_boundary(const std::string &content_type)
//			{
//				//用, 分割字符
//				auto vector = split_string(content_type, ", ");
//				//遍历集合
//				for (auto begin = std::begin(vector), end = std::end(vector); begin != end; begin++)
//				{
//					//如果为multipart/form-data则跳过
//					if (*begin == "multipart/form-data")
//					{
//						continue;
//					}
//					//找到boundary直接返回
//					auto pair = split_to_half(*begin, "=");
//					if (pair.first == "boundary")
//					{
//						return pair.second;
//					}
//				}
//				//找不到抛出异常
//				throw std::invalid_argument("无效的参数:无法找到boundary");
//			}
//			//表单数据类型
//			enum form_type
//			{
//				application_x_www_form_urlencoded = 0, //application/x-www-form-urlencoded
//				multipart_form_data, //multipart/form-data
//				text_plain, //text/plain
//				application_json //application/json
//			};
//			//获取表单数据类型
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
//				throw std::invalid_argument("未知的FormType");
//			}
//		}
//	}
//}