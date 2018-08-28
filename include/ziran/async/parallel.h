#pragma once
#include <vector>
#include <memory>
#include <initializer_list>
#include <functional>
#include <omp.h>
namespace ziran
{
	namespace async
	{
		//�첽�����
		template<typename T,typename TContainer = std::vector<T>>
		struct parallel_result
		{
			std::shared_ptr<TContainer> container_ptr;
			void set_result(std::shared_ptr<TContainer> ptr)
			{
				container_ptr = ptr;
			}
		};
		namespace parallel
		{
			//���е���
			template<
				//�������
				typename T
				//��������
				,typename TContainer = std::vector<T>
				//���������
				,typename TResult = parallel_result<T,TContainer>
			>
			TResult invoke(std::initializer_list<std::function<T()>> &&func_list)
			{
				//���������
				TResult result;
				//��������ָ��
				std::shared_ptr<TContainer> container_ptr = std::make_shared<TContainer>();
				//��ȡ��С
				size_t size = func_list.size();
				//OpenMP forѭ������
				#pragma omp parallel for schedule(dynamic)
				for (int i = 0; i < size; i++)
					container_ptr->push_back((*(func_list.begin() + i))());

				//���ý��
				result.set_result(container_ptr);
				//���ؽ��
				return result;
			}

			//���е���
			void invoke_void(std::initializer_list<std::function<void()>> &&func_list)
			{
				//��ȡ��С
				size_t size = func_list.size();
				//OpenMP for ѭ������
				#pragma omp parallel for schedule(dynamic)
				for (int i = 0; i < size; i++)
					(*(func_list.begin() + i))();
				return;
			}
			//����for_each
			template<
				//����
				typename T
				//��������
				,typename TContainer = std::vector<T>
			>
			void for_each(const TContainer &container,std::function<void(T&)> &&func)
			{
				//��ȡ��С
				size_t size = container.size();
				//OpenMP forѭ������
				#pragma omp parallel for schedule(dynamic)
				for (int i = 0; i < size; i++)
				{
					func(container[i]);
				}
			}
			//���е���
			void for_invoke(std::function<void()> &&func,size_t &&count)
			{
				//OpenMP forѭ������
				#pragma omp parallel for schedule(dynamic)
				for (int i = 0; i < count; i++)
				{
					func();
				}
			}
		}
	}
}