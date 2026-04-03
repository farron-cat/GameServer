
#include <iostream>

#include "network/network.h"

int main(int argc, char *argv[])
{
	//初始化Socket
	_sock_init();

	//创建Socket
	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		std::cout << "::socket failed. err:" << _sock_err() << std::endl;
		return 1;
	}

	//设定IP和端口并绑定
	//sockaddr_in结构体（Inernet环境下套接字地址形式），指定协议簇sin_family、IP地址sin_addr、端口sin_port
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2233);
	::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
	//绑定IP和端口
	if (::bind(socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
	{
		std::cout << "::bind failed. err:" << _sock_err() << std::endl;
		return 1;
	}

	//打开对Socket的监听
	int backlog = GetListenBacklog();
	if (::listen(socket, backlog) < 0)
	{
		std::cout << "::listen failed." << _sock_err() << std::endl;
		return 1;
	}

	//等待连接
	struct sockaddr socketClient;
	socklen_t socketLength = sizeof(socketClient);
	int newSocket = ::accept(socket, &socketClient, &socketLength); //在此处阻塞，直到::accept接收到一个请求

	std::cout << "accept one connection" << std::endl;

	//接收与发送
	char buf[1024];
    memset(&buf, 0, sizeof(buf));

	auto size = ::recv(newSocket, buf, sizeof(buf), 0); //接收数据
	if (size > 0)
	{
		std::cout << "::recv." << buf << std::endl;
		::send(newSocket, buf, size, 0); //发送数据
		std::cout << "::send." << buf << std::endl;
	}

	//关闭Socket
	_sock_close(socket);
	_sock_exit();

	std::getchar();
	return 0;
}

