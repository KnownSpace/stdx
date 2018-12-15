#pragma once

namespace stdx
{
	namespace net
	{
		//IP���ײ�
		struct ip_header
		{
			//�汾���ײ�����
			unsigned char version_and_header_length;;
			//TOS
			unsigned char tos;
			//�ܳ���
			unsigned short total_length;
			//��ʶ
			unsigned short id;
			//����
			unsigned short flag_and_offset;
			//����ʱ��
			unsigned char ttl;
			//Э��
			unsigned char protocol;
			//У���
			unsigned short checksum;
			//Դ��ַ
			unsigned int src;
			//Ŀ�ĵ�ַ
			unsigned int des;
		};
	}
}