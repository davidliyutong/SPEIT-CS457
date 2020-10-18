//
// Created by liyutong on 2020/10/5.
//
#include "../common/StorageDaemon.h"
#include "PeerDaemon.h"
#include "TcpTrackerMain.h"

int main(int argc, char *argv[]) {
    int ClientSock;
    struct sockaddr_in ClientAddr{};
    socklen_t nClientAddrLen;

    // 启动线程管理器
    TrackerConnectionDaemon ConnD;
    PeerDaemon PeerD;

    // 创建并绑定Socket到特定端口
    int SockServerFD = BindLocalTCPSocket(TRACKERPORT, false);
    if (SockServerFD < 0) {
        printf("Socket init failed \n");
        exit(1);
    }

    cout << "Start listening at " << TRACKERPORT << endl;

    // 开始监听
    listen(SockServerFD, SOCKET_WAIT_LEN);
    memset(&ClientAddr, 0, sizeof(ClientAddr));

    while (true) {
        // 接受连接
        ClientSock = accept(SockServerFD, (struct sockaddr *) &ClientAddr, &nClientAddrLen);
        if (ClientSock != -1) {
            PeerD.m_Loop(); // Peer管理器删除掉线的Peer
            ConnD.m_Loop(); // 线程管理器释放那些已经结束的线程
            ConnD.m_AddConnection(ClientSock, ClientAddr, PeerD); // 线程管理器新建一个线程以应对该Client

        }
    }
    return 0;

}