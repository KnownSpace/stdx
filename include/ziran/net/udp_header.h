#pragma once
#include <bitset>

namespace ziran
{
	namespace net
	{
		//UDP�ײ�
		struct udp_header
		{
			//Դ�˿�
			unsigned short src;

			//Ŀ�Ķ˿�
			unsigned short des;

			//������
			unsigned short length;

			//У���
			unsigned short checksum;
		};
	}
}