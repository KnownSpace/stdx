#include <iostream>
#include <ziran/cmder.h>
#include <ziran/windows/device.h>
#include <ziran/async/task.h>
int main()
{
	std::cout << "ʹ��ǰ���ȴ�USB!"<<std::endl;
	std::cout << "������Ҫж�ص��̷�(��: H: ):" << std::endl;
	std::string str;
	std::cin >> str;
	try
	{
		ziran::win::uninstall_usb(str);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what()<<std::endl;
	}
	ziran::cmder::pause();
	std::cin.get();
	return 0;
}