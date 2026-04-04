
#include <list>
#include <memory>
#include "client_socket.h"

int main(int argc, char *argv[])
{
	//创建3个阻塞式客户端 0、1、2
	std::list<std::shared_ptr<ClientSocket>> clients;
	for (int index = 0; index < 3; index++)
	{
		clients.push_back(std::make_shared<ClientSocket>(index));
	}

	while (!clients.empty())
	{
		//遍历clients，客户端线程为运行状态时继续，停止状态则关闭线程并将其移出
		auto iter = clients.begin();
		while (iter != clients.end())
		{
			auto pClient = (*iter);
			if ((*pClient).IsRun())
			{
				++iter;
				continue;
			}

			pClient->Stop();
			iter = clients.erase(iter);
		}
	}

	std::getchar();

	return 0;
}

