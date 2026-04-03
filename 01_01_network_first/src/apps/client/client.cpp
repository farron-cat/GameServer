

#include <iostream>

#include "network/network.h"

int main(int argc, char *argv[])
{
	//初始化Socket
	_sock_init();

	//创建Socket
	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //IPv4，有序可靠数据流，TCP协议
	if (socket == INVALID_SOCKET)
	{
		std::cout << "::socket failed. err:" << _sock_err() << std::endl;
		return 1;
	}

	//设定IP和端口
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2233);
	::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
	//发起连接操作
	if (::connect(socket, (struct sockaddr *)&addr, sizeof(sockaddr)) < 0)
	{
		std::cout << "::connect failed. err:" << _sock_err() << std::endl;
		return 1;
	}

	//发送字符串"ping"
	std::string msg = "ping";
	::send(socket, msg.c_str(), msg.length(), 0);

	std::cout << "::send." << msg.c_str() << std::endl;


	//接收数据
	char buffer[1024];
	memset(&buffer, 0, sizeof(buffer));
	::recv(socket, buffer, sizeof(buffer), 0);
	std::cout << "::recv." << buffer << std::endl;

	//关闭Socket
	_sock_close(socket);
	_sock_exit();

	std::getchar();
	return 0;
}

