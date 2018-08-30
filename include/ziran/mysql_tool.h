#pragma once
#include <ziran/encode_tool.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
namespace ziran
{
	namespace tools
	{
		namespace mysql
		{
			//����MYSQL����
			std::shared_ptr<sql::Connection> make_mysql_connection(const std::string &host_name, const std::string &user, const std::string &pwd)
			{
				//��ȡMYSQL����
				sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
				//����MYSQL���ݿ�
				auto *conn = driver->connect(host_name.c_str(), user.c_str(), pwd.c_str());
				//��������ָ��
				std::shared_ptr<sql::Connection> conn_ptr(conn, [](sql::Connection *conn) {
					if (!conn->isClosed())
					{
						conn->close();
					}
				});
				return conn_ptr;
			}
			//����MYSQL���� (���ذ汾)
			std::shared_ptr<sql::Connection> make_mysql_connection(const std::string &host_name, const std::string &user, const std::string &pwd, const std::string &db_name)
			{
				//����֮ǰ�İ汾
				auto conn = make_mysql_connection(host_name, user, pwd);
				//����Ҫʹ�õ����ݿ�
				conn->setSchema(db_name.c_str());
				//��������
				return conn;
			}
		}
	}
}