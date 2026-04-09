#pragma once

#include "network.h"
#include <string>

/// <summary>
/// @brief 服务端监听，Listen()绑定端口，Accept()接受连接
/// </summary>
class NetworkListen :public Network
{
public:
	bool Listen(std::string ip, int port);
	bool Update();

protected:
	virtual int Accept();
};
