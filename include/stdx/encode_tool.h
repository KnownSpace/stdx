#pragma once
#include <boost/locale.hpp>

//Լ��:MYSQLʹ��UTF8�ַ���
//Լ��:��ҳ���ʹ��UTF8�ַ���
//Լ��:Windowsʹ��GBK����
//��������UTF8ת��GBK
#define UTF_TO_GBK(str) boost::locale::conv::between(str,"gbk","utf8")
#ifdef WIN32 
//WIN32 �ַ���ΪGBK
//��ȡGBK�ַ��� ֱ�ӷ���
#define GBK(str) str 
//��ȡYTF8�ַ��� ����ת��
#define UTF8(str) boost::locale::conv::between(str,"utf8","gbk")
//MYSQL,�������ת�����ر��� UTF8תGBK
#define LOCAL(str) UTF_TO_GBK(str)
#else
//Linux �ַ���ΪUTF8
//��ȡUTF8�ַ��� ֱ�ӷ���
#define UTF8(str) str
//��ȡGBK�ַ��� ����ת��
#define GBK(str) boost::locale::conv::between(str,"gbk","utf8")
//MYSQL,�������ת�����ر��� ֱ�ӷ���
#define LOCAL(str) str
#endif