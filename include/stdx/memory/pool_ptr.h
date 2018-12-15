#pragma once
#include <atomic>
namespace ziran
{
	namespace memory
	{
		//�����ǰ������
		class object_pool;
		//��ָ��ģ��
		template<
			//����
			typename T
			//����������
			,typename TCount = std::atomic_int
			//���������
			,typename TPool = ziran::memory::object_pool
		>
		class pool_ptr
		{
		public:
			//���캯��
			explicit pool_ptr(T *ptr,TPool &pool)
				:ptr(ptr)
				,pool(pool)
			{
				count = new TCount;
				*count = 1;
			}
			//�������캯��
			pool_ptr(const pool_ptr<T, TCount, TPool> &other)
				:ptr(other.ptr)
				,count(other.count)
				,pool(other.pool)
			{
				*count += 1;
			}
			//move���캯��
			pool_ptr(pool_ptr<T, TCount, TPool> &&other)
				:ptr(std::move(other.ptr))
				,count(std::move(other.count))
				,pool(std::move(other.pool))
			{
			}
			//������ֵ����
			pool_ptr<T, TCount, TPool> &operator=(const pool_ptr<T, TCount, TPool> &other)
			{
				ptr = other.ptr;
				count = other.count;
				pool = other.pool;
			}
			//����*������
			T &operator*() const
			{
				return *ptr;
			}
			//����->������
			T *operator->() const
			{
				return ptr;
			}
			//���ص�bool��ת��
			operator bool() const
			{
				return ptr;
			}
			//��������
			~pool_ptr()
			{
				//������������ͷ�
				if (!count)
				{
					//ֱ�ӷ���
					return;
				}
				try
				{
					//���ü����1
					*count -= 1;
					//�������������0
					if (*count == 0)
					{
						//���ٶ���
						pool.destroy_object(ptr);
						//���ټ�����
						delete count;
						//������������
						count = nullptr;
					}
				}
				catch (const std::exception&)
				{
					return;
				}
			}
		private:
			//����ָ��
			T *ptr;
			//������ָ��
			TCount *count;
			//���������
			TPool &pool;
		};
	}
}