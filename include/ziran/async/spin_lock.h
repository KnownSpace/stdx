#pragma once
#include <atomic>
#include <chrono>
#include <thread>

namespace ziran
{
	namespace async
	{
		//������ģ��
		template<
			//����ʱ��(����)
			int sleep_milliseconds
		>
		class spin_lock
		{
		public:
			//Ĭ�Ϲ��캯��
			spin_lock()
				:is_using(false)
			{
			}
			//��������
			~spin_lock() = default;
			//������
			void enter()
			{
				//�����ռ��
				while (is_using)
				{
					//����������
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_milliseconds));
				}
				//�����
				//��״̬����Ϊ��ռ��
				is_using = true;
			}
			//�˳���
			void exit()
			{
				//��״̬����Ϊ����ռ��
				is_using = false;
			}
		private:
			//����״̬
			std::atomic_bool is_using;
		};
		//�ػ�ģ��
		//������ ������
		template<>
		class spin_lock<0>
		{
		public:
			//Ĭ�Ϲ��캯��
			spin_lock()
				:is_using(false)
			{
			}
			~spin_lock() = default;
			//������
			void enter()
			{
				//�����ռ��
				while (is_using)
				{
				}
				//�����
				//��״̬����Ϊ��ռ��
				is_using = true;
			}
			//�˳���
			void exit()
			{
				//��״̬����Ϊ����ռ��
				is_using = false;
			}
		private:
			//����״̬
			std::atomic_bool is_using;
		};
	}
}