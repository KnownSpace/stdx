#pragma once

namespace ziran
{
	namespace net
	{
		//α�ײ�
		struct pseudo_header
		{
			//Դ��ַ
			unsigned int src;

			//Ŀ�ĵ�ַ
			unsigned int des;

			//���������
			unsigned char placeholder;

			//Э��
			unsigned char protocol;

			//TCP/UDP�ײ�����
			unsigned short tcp_udp_header_length;
		};
	}
}