//
// Created by liyutong on 2020/10/5.
//

#ifndef P2P_FILE_SHARE_TCPSERVERMAIN_H
#define P2P_FILE_SHARE_TCPSERVERMAIN_H

#include <cstring>
#include <arpa/inet.h>
#include "../common/MsgInterpreter.h"
#include "../common/data.h"
#include "../common/macros.h"
#include "../common/ConnectionDaemon.h"

using namespace std;

void
HandleConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, queue<int> &a_MsgQueue, StorageDaemon &a_Storage) {
    /*
     * 这个函数负责与CLient通讯
     * a_ClientSock 是监听线程accept得到的Socket
     * a_ClientAddr 是客户端的IP-Port二元组
     * a_MsgQueue 是线程用于和线程管理器交换信息的队列
     * a_Storage 是存储管理器对象
     */

    // 展示Client信息
    printf("[%d] New connection from %s:%d\n", a_ClientSock, inet_ntoa(a_ClientAddr.sin_addr),
           ntohl(a_ClientAddr.sin_port));
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
    int nReadLen;
    bool bIsConnectionFinished = false;

    // 这里粗暴地用一个try结构来提供容错，防止Client在发送中途掉线（BROKEN PIPE）导致异常
    try {
        while (true) {

            // 如果Client掉线或者双方沟通后结束连接
            if (not SocketConnected(a_ClientSock) or bIsConnectionFinished) {
                break;
            }

            // 从Client接受控制信息
            memset(RecvBuf, 0, sizeof(char) * SENDBUFFLEN);
            SetBlock(a_ClientSock);
            nRecvLen = recv(a_ClientSock, &RecvBuf, CRTLMSGLEN, 0);

            // 如果收到信息
            if (nRecvLen > 0) {
                // 解析信息
                CM = Intpt.m_interpret(RecvBuf, nRecvLen);
                // 根据解析结果，确定动作
                switch (CM.type) {
                    case 'N': { // 不完整的信息
                        break;
                    }
                    case 'Q': { // Client主动结束
                        bIsConnectionFinished = true;
                        break;
                    }
                    case 'E' : { // Client内部出错
                        printf("[%d] There is an error\n", a_ClientSock);fflush(stdout);
                        fflush(stdout);
                        bIsConnectionFinished = true;
                        break;
                    }
                    case 'G': { // Client想要文件
                        // 显示文件名
                        printf("[%d] Content: %s \n", a_ClientSock, CM.content.c_str());fflush(stdout);
                        fflush(stdout);

                        // 获取文件长度
                        int nFileLen = a_Storage.m_FileExist(CM.content);
                        if (nFileLen > 0) { // 文件存在
                            // 打开文件待读
                            FILE *pFin;
                            pFin = fopen(a_Storage.m_CompletePath(CM.content).c_str(), "rb");
                            if (pFin != nullptr) { // 成功打开文件
                                // 发送文件长度
                                FormulateFileLenResp(nFileLen, SendBuf, SENDBUFFLEN);
                                nSendLen = send(a_ClientSock, &SendBuf, CRTLMSGLEN, 0);
                                printf("[%d] FileLen: [%d] byte\n", a_ClientSock, nFileLen);

                                // 循环读取文件并发送
                                int nByteLeft = nFileLen;
                                while (nByteLeft > 0) {
                                    if (nByteLeft >= FILEBUFFLEN) {
                                        nReadLen = fread(&FileBuf, sizeof(char), FILEBUFFLEN, pFin);
                                        nSendLen = send(a_ClientSock, &FileBuf, FILEBUFFLEN, 0);
                                    } else {
                                        nReadLen = fread(&FileBuf, sizeof(char), nByteLeft, pFin);
                                        nSendLen = send(a_ClientSock, &FileBuf, nByteLeft, 0);
                                    }
                                    nByteLeft -= nSendLen;
                                }
                                printf("[%d] Sending %d byte of data\n", a_ClientSock, nFileLen - nByteLeft);fflush(stdout);
                                fflush(stdout);
                            } else { // 无法打开文件
                                // 回复Client错误
                                ReplyClientWithError(a_ClientSock);
                                printf("[%d] Fail to read the file\n", a_ClientSock);fflush(stdout);
                                break;
                            }
                        } else { // 文件不存在
                            // 回复Client错误
                            ReplyClientWithError(a_ClientSock);
                            printf("[%d] File not found\n", a_ClientSock);fflush(stdout);
                        }
                        break;
                    }
                    case 'I': { // Client想要列出目录索引
                        printf("[%d] Dir: %s \n", a_ClientSock, CM.content.c_str());fflush(stdout);

                        // 得到目录索引信息
                        string CurrIndex = a_Storage.m_ListDirDisp(CM.content);

                        if (CurrIndex.length() > 0) { // 目录存在
                            uint64_t nCurrIndexLen = CurrIndex.length();
                            cout << CurrIndex << endl;

                            // 发送索引长度
                            memset(SendBuf, 0, sizeof(char) * SENDBUFFLEN);
                            SendBuf[0] = 'Y';
                            Uint64ToChar(nCurrIndexLen, &SendBuf[1]);
                            send(a_ClientSock, &SendBuf, CRTLMSGLEN, 0);

                            // 发送索引
                            send(a_ClientSock, CurrIndex.c_str(), nCurrIndexLen, 0);
                            printf("[%d] Sending %d byte of data\n", a_ClientSock, nSendLen);fflush(stdout);
                        } else { // 目录不存在
                            ReplyClientWithError(a_ClientSock);
                            printf("[%d] Dir not found\n", a_ClientSock);fflush(stdout);
                        }
                        break;
                    }
                    default: {
                        // 收到了客户端损坏的数据
                        printf("[%d] Data corrupted\n", a_ClientSock);fflush(stdout);
                    }
                }
            } else if (nRecvLen == -1) { // 未收到数据
                printf("[%d] No incoming message\n", a_ClientSock);fflush(stdout);
                break;
            } else {
                usleep(5);
                continue;
            }
        }
    } catch (const char *err) { // 如果客户端在接受过程中意外关闭管道，会产生错误
        printf("[%d] Unknown error...\n", a_ClientSock);fflush(stdout);
    }

    // 向队列中添加当前socket，从而通知线程管理器该线程已经运行结束
    a_MsgQueue.push(a_ClientSock);
    printf("[%d] Exitting...\n", a_ClientSock);
    fflush(stdout);
}

class ServerConnectionDaemon:public ConnectionDaemon {
    /*
     * 这个类继承了ConnectionDaemon类，重写了m_AddConenction方法来添加自定义的任务
     */
public:
    void m_AddConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, StorageDaemon &a_Storage) {
        auto *pThread = new thread(HandleConnection, a_ClientSock, a_ClientAddr, ref(qMsgQueue), ref(a_Storage));
        this->mapActiveClients[a_ClientSock] = pThread;
    }

};



#endif //P2P_FILE_SHARE_TCPSERVERMAIN_H
