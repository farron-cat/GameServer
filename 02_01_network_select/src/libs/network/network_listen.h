#pragma once

#include "network.h"
#include <string>

class NetworkListen :public Network
{
public:
	bool Listen(std::string ip, int port);
	bool Update();

protected:
	virtual int Accept();
};
