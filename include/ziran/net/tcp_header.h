#pragma once

namespace ziran
{
	namespace net
	{
		//TCP�ײ�
		struct tcp_header
		{
			//Դ�˿�
			unsigned short src;
			//Ŀ�Ķ˿�
			unsigned short des;
			//���
			unsigned int seq;
			//ȷ�Ϻ�
			unsigned int ack;
			//����ƫ��
			unsigned char offset_and_res;
			//��־
			//0
			//0
			//SYN
			//ACK
			//FIN
			//URG
			//PSH
			//RST
			unsigned char res_and_flag;
			//����
			unsigned short window;
			//У���
			unsigned short checksum;
			//����ָ��
			unsigned short urg_ptr;
		};
	}
}