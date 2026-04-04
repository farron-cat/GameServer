#include "client_socket.h"
#include "network/network.h"
#include <string>
#include <random>

ClientSocket::ClientSocket(int index) : _curIndex(index)
{
	//创建线程
	_thread = std::thread([index, this]()
	{
		_isRun = true; 
		this->MsgHandler(); //执行阻塞式收发数据流程
		_isRun = false;
	});
}

bool ClientSocket::IsRun() const
{
	return _isRun;
}

void ClientSocket::Stop()
{
	if (_thread.joinable())
		_thread.join();
}


void ClientSocket::MsgHandler()
{
	//初始化并创建Socket
	_sock_init();
	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		std::cout << "::socket failed. err:" << _sock_err() << std::endl;
		return;
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
		return;
	}

	//发送数据并接收
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));

	std::string msg = "ping_" + std::to_string(_curIndex);
	std::cout << "::send." << msg.c_str() << " socket:" << socket << std::endl;
	::send(socket, msg.c_str(), msg.length(), 0);

	memset(&buffer, 0, sizeof(buffer));
	::recv(socket, buffer, sizeof(buffer), 0);
	std::cout << "::recv." << buffer << " socket:" << socket << std::endl << std::endl;

	//关闭Socket
	_sock_close(socket);
	_sock_exit();
}

