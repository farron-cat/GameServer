#pragma once
#include <thread>

class ClientSocket
{
public:
	ClientSocket(int index);
	void MsgHandler(); //阻塞式收发数据流程
	bool IsRun() const; //收发数据是否已经完成
	void Stop(); //结束线程

private:
	bool _isRun{ true };
	int _curIndex;
	std::thread _thread;
};

