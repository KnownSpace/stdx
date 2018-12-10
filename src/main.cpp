#include <iostream>
#include <ziran/cmder.h>
#include <ziran/windows/device.h>
#include <ziran/async/task.h>
int main()
{
	TASK<void> t = MAKE_TASK<void>([]() {});
	t->then([]() 
	{
		std::cout << "1";
	})->then([]() 
	{
		auto t =MAKE_TASK<void>([]() {
			std::cout << "2";
		});
		return t;
	})
		->then([]() {
		std::cout << "3";
	});
	//std::cout << "ʹ��ǰ���ȴ�USB!"<<std::endl;
	//std::cout << "������Ҫж�ص��̷�(��: H: ):" << std::endl;
	//std::string str;
	//std::cin >> str;
	//try
	//{
	//	ziran::win::uninstall_usb(str);
	//}
	//catch (const std::exception& e)
	//{
	//	std::cout << e.what()<<std::endl;
	//}
	ziran::cmder::pause();
	return 0;
}