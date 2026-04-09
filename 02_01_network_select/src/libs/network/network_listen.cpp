#include <iostream>

#include "network_listen.h"
#include "connect_obj.h"

bool NetworkListen::Listen(std::string ip, int port) {
    //创建_masterSocket用于监听
    _masterSocket = CreateSocket();
    if (_masterSocket == INVALID_SOCKET)
        return false;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);

    if (::bind(_masterSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cout << "::bind failed. err:" << _sock_err() << std::endl;
        return false;
    }

    int backlog = 10;
#ifndef WIN32
    char* ptr;
    if ((ptr = getenv("LISTENQ")) != nullptr)
        backlog = atoi(ptr);
#endif

    if (::listen(_masterSocket, backlog) < 0) {
        std::cout << "::listen failed." << _sock_err() << std::endl;
        return false;
    }

    return true;
}

int NetworkListen::Accept() {
    struct sockaddr socketClient;
    socklen_t socketLength = sizeof(socketClient);

    int rs = 0;
    //使用while取尽所有连接
    while (true) {
        //获取新连接
        SOCKET socket = ::accept(_masterSocket, &socketClient, &socketLength);
        if (socket == INVALID_SOCKET)
            return rs;
        //为新建Socket设置参数
        SetSocketOpt(socket);

        //新建ConnectObj用于管理当前连接，同时将其加入队列中
        ConnectObj* pConnectObj = new ConnectObj(this, socket);
        _connects.insert(std::make_pair(socket, pConnectObj));

        ++rs;
    }
}

bool NetworkListen::Update() {
    //调用函数对所有连接的Socket进行检测
    bool br = Select();

    //监听_masterSocket的有可读消息，说明有新连接并接受
    if (FD_ISSET(_masterSocket, &readfds)) {
        Accept();
    }

    return br;
}
