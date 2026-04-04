#pragma once

#include "disposable.h"

#include <map>

#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#define SOCKET int
#define INVALID_SOCKET -1

#define _sock_init( )
#define _sock_nonblock( sockfd ) { int flags = fcntl(sockfd, F_GETFL, 0); fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); }
#define _sock_exit( )
#define _sock_err( )	errno
#define _sock_close( sockfd ) ::close( sockfd )
#define _sock_is_blocked()	(errno == EAGAIN || errno == 0)

#else

#define FD_SETSIZE      1024

#include <Ws2tcpip.h>
#include <windows.h>

#define _sock_init( )	{ WSADATA wsaData; WSAStartup( MAKEWORD(2, 2), &wsaData ); }
#define _sock_nonblock( sockfd )	{ unsigned long param = 1; ioctlsocket(sockfd, FIONBIO, (unsigned long *)&param); }
#define _sock_exit( )	{ WSACleanup(); }
#define _sock_err( )	WSAGetLastError()
#define _sock_close( sockfd ) ::closesocket( sockfd )
#define _sock_is_blocked()	(WSAGetLastError() == WSAEWOULDBLOCK)

#endif

class ConnectObj;
/// <summary>
/// @brief Network基类，管理socket和Select()多路复用
/// </summary>
class Network : public IDisposable {
public:
	void Dispose( ) override;
	bool Select();

	SOCKET GetSocket( ) const{ return _masterSocket; }
protected:
	static void SetSocketOpt(SOCKET socket);
	SOCKET CreateSocket( );

protected:
	SOCKET _masterSocket{ INVALID_SOCKET }; //在每一帧的循环中，网络系统都会监视 _masterSocket 的状态变化（如可读、可写或异常），从而驱动整个网络层的后续操作
	std::map<SOCKET, ConnectObj*> _connects; //监听类NetworkListen：有无数个ConnectObj类，保存每个连接的数据；连接类NetworkConnector：只有一个ConnectObj类
	fd_set readfds, writefds, exceptfds; //三个fd_set集合，分别用于存储可读、可写、异常的描述符
};
