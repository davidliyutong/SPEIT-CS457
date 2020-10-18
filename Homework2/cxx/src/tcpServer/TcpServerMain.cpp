#include <iostream>
#include <map>
#include <thread>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common/utils.h"
#include "../common/StorageDaemon.h"
#include "TcpServerMain.h"


using namespace std;


int main(int argc, char *argv[]) {
    int ClientSock;
    struct sockaddr_in ClientAddr{};
    socklen_t nClientAddrLen;

    // 启动线程管理器
    ServerConnectionDaemon ConnD;

    // 设置服务器根目录
    string RootPath;
    if (argc < 2) {
        RootPath.assign("./Files/");
    } else {
        RootPath.assign(argv[1]);
    }
    // 设置存储管理器
    StorageDaemon StorageD(RootPath);

    // 创建并绑定Socket到特定端口
    int SockServerFD = BindLocalTCPSocket(SERVERPORT, false);
    if (SockServerFD < 0) {
        printf("Socket init failed \n");
        fflush(stdout);
        exit(1);
    }

    cout << "Root dir: " << RootPath << endl;
    cout << "Start listening at: " << SERVERPORT << endl;
    fflush(stdout);

    // 开始监听
    listen(SockServerFD, SOCKET_WAIT_LEN);

    while (true) {
        // 接受连接
        ClientSock = accept(SockServerFD, (struct sockaddr *) &ClientAddr, &nClientAddrLen);
        if (ClientSock != -1) {
            ConnD.m_Loop(); // 线程管理器释放那些已经结束的线程
            ConnD.m_AddConnection(ClientSock, ClientAddr, StorageD); // 线程管理器新建一个线程以应对该Client
        }
    }
    return 0;
}