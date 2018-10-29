#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace ziran
{
	namespace async
	{
		//����
		class barrier
		{
		public:
			//Ĭ�Ϲ��캯��
			barrier()
				:mutex(std::make_shared<std::mutex>())
				,notify_count(0)
				,cv(std::make_shared<std::condition_variable>())
			{}

			//���캯��
			barrier
			(
				//��ʼ��ͨ������
				const int &pass_count
			)
				:mutex(std::make_shared<std::mutex>())
				,notify_count(pass_count)
				,cv(std::make_shared<std::condition_variable>())
			{}
			//��������
			~barrier()
			{
			}

			//�ȴ�ͨ��
			void wait()
			{
				std::unique_lock<std::mutex> lock(*mutex);
				auto &n = notify_count;
				cv->wait(lock, [&n]() { return (int)n; });
				notify_count -= 1;
			}
			//ͨ��
			void pass()
			{
				cv->notify_one();
				notify_count += 1;
			}
				
		private:
			std::shared_ptr<std::mutex> mutex;
			std::atomic_int notify_count;
			std::shared_ptr<std::condition_variable> cv;
		};
		using shared_barrier = std::shared_ptr<ziran::async::barrier>;
		using unique_barrier = std::unique_ptr<ziran::async::barrier>;
	}
}