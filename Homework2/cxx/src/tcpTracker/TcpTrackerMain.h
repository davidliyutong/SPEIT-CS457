//
// Created by liyutong on 2020/10/5.
//

#ifndef P2P_FILE_SHARE_TCPTRACKERMAIN_H
#define P2P_FILE_SHARE_TCPTRACKERMAIN_H

#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <map>
#include <unistd.h>
#include "PeerDaemon.h"
#include "../common/MsgInterpreter.h"
#include "../common/data.h"
#include "../common/utils.h"
#include "../common/macros.h"
#include "../common/ConnectionDaemon.h"

using namespace std;


void HandleConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, queue<int> &a_MsgQueue, PeerDaemon &a_PeerD) {
    /*
     * 这个函数用于和Node通讯：
     * - 接受Node注册
     * - 向Node通报Peer信息
     */
    printf("[%d] New connection from %s:%d\n", a_ClientSock, inet_ntoa(a_ClientAddr.sin_addr),
           ntohs(a_ClientAddr.sin_port));

    // 对于无法解析的地址，服务器不做记录
    if (ntohs(a_ClientAddr.sin_port) == 0) {
        ReplyClientWithError(a_ClientSock);
        fflush(stdout);
        return;
    }
    fflush(stdout);
    // 设置Socket的发送超时
    SetSockSendTimeout(10, 0, a_ClientSock);

    MsgInterpreter Intpt;
    ClientMsg CM;
    char RecvBuf[RECVBUFFLEN];
    char SendBuf[SENDBUFFLEN];
    char FileBuf[FILEBUFFLEN];
    int nRecvLen;
    int nSendLen;
    bool bIsConnectionFinished = false;

    // 这里粗暴地用一个try结构来提供容错
    try {
        while (true) {

            // 如果Client掉线或者双方沟通后结束连接
            if (not SocketConnected(a_ClientSock) or bIsConnectionFinished) {
                break;
            }

            // 从Client接受控制信息
            memset(RecvBuf, 0, sizeof(char) * RECVBUFFLEN);
            SetBlock(a_ClientSock);
            nRecvLen = recv(a_ClientSock, &RecvBuf, CRTLMSGLEN, 0);

            // 如果收到信息
            if (nRecvLen > 0) {
                // 解析信息，这里用了比较直接的方法，因为在macro.h里定义了控制命令的长度为512字节
                switch (RecvBuf[0]) {
                    case 'N': { // 不完整的信息
                        break;
                    }
                    case 'Q': { // Client主动结束
                        bIsConnectionFinished = true;
                        break;
                    }
                    case 'E' : { // Client内部出错
                        printf("[%d] There is an error\n", a_ClientSock);
                        fflush(stdout);
                        bIsConnectionFinished = true;
                        break;
                    }
                    case 'R': { // Client想要注册
                        printf("[%d] New registration from %s:%d\n", a_ClientSock, inet_ntoa(a_ClientAddr.sin_addr),
                               ntohs(a_ClientAddr.sin_port));

                        // 完成注册，返回成功信息（这里应该做一个判断）
                        a_PeerD.m_AddPeer((sockaddr &) a_ClientAddr);
                        memset(SendBuf, 0, sizeof(char) * SENDBUFFLEN);
                        SendBuf[0] = 'Y';
                        SendBuf[CRTLMSGLEN - 1] = '\n';
                        nSendLen = send(a_ClientSock, SendBuf, CRTLMSGLEN, 0);
                        break;

                    }
                    case 'P': { // Client想要Peers信息
                        printf("[%d] New peer query from %s:%d\n", a_ClientSock, inet_ntoa(a_ClientAddr.sin_addr),
                               ntohs(a_ClientAddr.sin_port));

                        // 查询
                        string res = a_PeerD.m_QueryPeer((sockaddr &) a_ClientAddr);
                        int nInfoLen = res.length();

                        // 发送Peers的长度
                        FormulateFileLenResp(nInfoLen, SendBuf, SENDBUFFLEN);
                        nSendLen = send(a_ClientSock, &SendBuf, CRTLMSGLEN, 0);
                        printf("[%d] PeerInfoLen: [%d] byte\n", a_ClientSock, nInfoLen);

                        // 发送Peers信息
                        nSendLen = send(a_ClientSock, res.c_str(), nInfoLen, 0);
                        printf("[%d] Sending %d byte of data\n", a_ClientSock, nSendLen);
                        break;
                    }
                    default: { // 数据损失
                        // Data corruption
                        printf("[%d] Data corrupted\n", a_ClientSock);
                    }
                }
            } else if (nRecvLen == -1) { // 未收到信息
                printf("[%d] No incoming message\n", a_ClientSock);
                break;
            } else {
                usleep(50);
                continue;
            }
        }
    } catch (const char *err) { // 如果客户端在接受过程中意外关闭管道，会产生错误
        printf("[%d] Unknown error...\n", a_ClientSock);
    }

    // 向队列中添加当前socket，从而通知线程管理器该线程已经运行结束
    a_MsgQueue.push(a_ClientSock);
    printf("[%d] Exitting...\n", a_ClientSock);
    // 关闭Socket
    close(a_ClientSock);
    fflush(stdout);
}

class TrackerConnectionDaemon: public ConnectionDaemon {
    /*
 * 这个类继承了ConnectionDaemon类，重写了m_AddConenction方法来添加自定义的任务
 */
public:
    void m_AddConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, PeerDaemon &a_PeerD) {
        auto *pThread = new thread(HandleConnection, a_ClientSock, a_ClientAddr, ref(qMsgQueue), ref(a_PeerD));
        mapActiveClients[a_ClientSock] = pThread;
    }
};

#endif //P2P_FILE_SHARE_TCPSERVERMAIN_H
