#include "network.h"
#include "connect_obj.h"

#include <iostream>

//资源释放，override自IDisposable
void Network::Dispose()
{
	auto iter = _connects.begin();
	while (iter != _connects.end())
	{
		iter->second->Dispose();
		delete iter->second;
		++iter;
	}
	_connects.clear();

	//std::cout << "network dispose. close socket:" << _socket << std::endl;
	_sock_close(_masterSocket);
	_masterSocket = -1;
}

#ifndef WIN32
#define SetsockOptType void *
#else
#define SetsockOptType const char *
#endif

void Network::SetSocketOpt(SOCKET socket)
{
	// 1.端口关闭后马上重新启用
	bool isReuseaddr = true;
	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (SetsockOptType)&isReuseaddr, sizeof(isReuseaddr));

	// 2.发送、接收timeout
	int netTimeout = 3000; // 1000 = 1秒
	setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));

#ifndef WIN32

	int keepAlive = 1;
	socklen_t optlen = sizeof(keepAlive);

	int keepIdle = 60 * 2;	// 在socket 没有交互后 多久 开始发送侦测包
	int keepInterval = 10;	// 多次发送侦测包之间的间隔
	int keepCount = 5;		// 侦测包个数

	setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (SetsockOptType)&keepAlive, optlen);
	if (getsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, &optlen) < 0)
	{
		std::cout << "getsockopt SO_KEEPALIVE failed." << std::endl;
	}

	setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, optlen);
	if (getsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &keepIdle, &optlen) < 0)
	{
		std::cout << "getsockopt TCP_KEEPIDLE failed." << std::endl;
	}

	setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, optlen);
	setsockopt(socket, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, optlen);

#endif

	// 3.非阻塞
	_sock_nonblock(socket);
}

SOCKET Network::CreateSocket()
{
	_sock_init();
	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		std::cout << "::socket failed. err:" << _sock_err() << std::endl;
		return socket;
	}

	SetSocketOpt(socket);
	return socket;
}

bool Network::Select()
{
	//初始化所有集合，FD_ZERO宏用于清空集合
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	//将_masterSocket放入集合中
	//程序通过_masterSocket绑定 IP 和端口并开启监听
	//当底层函数::select检测到 _masterSocket 有读事件时，表示有新的客户端请求连接，随后程序会调用::accept 来接收这个连接
	FD_SET(_masterSocket, &readfds);
	FD_SET(_masterSocket, &writefds);
	FD_SET(_masterSocket, &exceptfds);

	//记录最大Socket值
	SOCKET fdmax = _masterSocket;
	
	//遍历所有连接，将所有有效的Socket加入上面三个集合中
	for (auto iter = _connects.begin(); iter != _connects.end(); ++iter)
	{
		//更新最大Socket值
		if (iter->first > fdmax)
			fdmax = iter->first;

		//监控这个Socket是否可读（接收数据）和发生异常
		FD_SET(iter->first, &readfds);
		FD_SET(iter->first, &exceptfds);

		//如果这个Socket已经发送数据，则监控其是否可以写（发送数据）
		if (iter->second->HasSendData()) {
			FD_SET(iter->first, &writefds);
		}
		else
		{
			if (_masterSocket == iter->first)
				FD_CLR(_masterSocket, &writefds);
		}
		/*在 select 网络模型中，Socket 加入可写集合（writefds）的处理逻辑不同于可读（readfds）和异常（exceptfds）集合，这主要是基于性能优化和按需驱动的原则：
			1.避免“忙等待”现象：在正常网络状态下，只要系统发送缓冲区未满，Socket 几乎总是处于“可写”状态.如果像处理可读集合那样无条件地将所有 Socket 加入可写集合，::select 函数会因为 Socket 总是可写而立即返回，导致每一帧都触发发送逻辑，造成严重的 CPU 资源浪费（忙等待）  
			2.按需加入逻辑：代码中通过 iter->second->HasSendData() 进行判断，只有在本地发送缓冲区（SendNetworkBuffer）确实有待发送的数据时，才会将该 Socket 放入 writefds 集合
			3.精确驱动：这种设计确保了只有在“应用层有数据要发”且“底层系统准备好接收数据”这两个条件同时满足时，才会触发真正的 ::send 调用，从而提高了网络 IO 的处理效率
		*/

	}

	//设定timeout参数结构体
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50 * 1000;
	//调用select函数从集合中筛选
	int nfds = ::select(fdmax + 1, &readfds, &writefds, &exceptfds, &timeout);
	if (nfds <= 0)
		return true;
	//返回值大于0，表示三个集合中有数据
	//遍历所有连接，看看是否有数据改变
	auto iter = _connects.begin();
	while (iter != _connects.end())
	{
		//检查是否有Socket发生异常
		if (FD_ISSET(iter->first, &exceptfds))
		{
			std::cout << "socket except!! socket:" << iter->first << std::endl;
			//发生异常，关闭这个Socket
			iter->second->Dispose();
			delete iter->second;
			iter = _connects.erase(iter);
			continue;
		}


		//检查是否有Socket的数据需要读取
		if (FD_ISSET(iter->first, &readfds))
		{
			if (!iter->second->Recv())
			{
				//读取失败，关闭这个Socket
				iter->second->Dispose();
				delete iter->second;
				iter = _connects.erase(iter);
				continue;
			}
		}

		//检查是否有Socket允许发送数据
		if (FD_ISSET(iter->first, &writefds))
		{
			if (!iter->second->Send())
			{
				//发送失败，关闭这个Socket
				iter->second->Dispose();
				delete iter->second;
				iter = _connects.erase(iter);
				continue;
			}
		}

		++iter;
	}

	return true;
}
