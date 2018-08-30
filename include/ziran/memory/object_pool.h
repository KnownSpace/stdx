#pragma once
#include <memory>
#include <ziran/memory/pool_ptr.h>
namespace ziran
{
	namespace memory
	{
		//�����
		class object_pool
		{
		public:
			//Ĭ�Ϲ��캯��
			object_pool() = default;
			//��������
			~object_pool() = default;
			//��������
			template<
				//����
				typename T
				//��������
				, typename ...TArgs
			>
			T *make_object(TArgs &&...args)
			{
				//��ȡ�����С
				auto size = sizeof(T);
				//�����ڴ�
				auto *byte_ptr = allocator.allocate(size);
				//ִ�й��캯��
				T *ptr = new(byte_ptr) T(args...);
				//����ָ��
				return ptr;
			}
			//������ָ��
			template<
				//����
				typename T
				//��ָ������
				,typename TPtr=ziran::memory::pool_ptr<T>
			>
			TPtr make_pool_ptr(T *ptr)
			{
				return TPtr(ptr, *this);
			}
			//������ָ��
			template<
				//����
				typename T
				//��������
				typename ...TArgs
			>
			ziran::memory::pool_ptr<T> make_object_ptr(TArgs &&...args)
			{
				return make_pool_ptr<T>(make_object<T>(args...));
			}

			//���ٶ���
			template <typename T>
			void destroy_object(T* ptr)
			{
				//��ȡ��С
				auto size = sizeof(T);
				//ִ����������
				ptr->~T();
				//�����ڴ�
				allocator.deallocate(reinterpret_cast<char*>(ptr), size);
			}
		private:
			std::allocator<char> allocator;
		};
	}
}