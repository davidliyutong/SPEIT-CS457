//
// Created by liyutong on 2020/10/11.
//
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <mutex>
#include <cstdlib>
#include "../common/utils.h"
#include "../common/StorageDaemon.h"
#include "../common/data.h"
#include "TcpNodeMain.h"


void
HandleConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, queue<int> &a_MsgQueue, StorageDaemon &a_Storage) {
    /*
     * 这个函数用于响应其他Node的请求（读取文件）
     */
    printf("[%d] New connection from %s:%d\n", a_ClientSock, inet_ntoa(a_ClientAddr.sin_addr),
           ntohs(a_ClientAddr.sin_port));
    fflush(stdout);
    // 设置Socket的发送超时
    SetSockSendTimeout(10, 0, a_ClientSock);


    char RecvBuf[SENDBUFFLEN];
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
            memset(RecvBuf, 0, sizeof(char) * SENDBUFFLEN);
            SetBlock(a_ClientSock);
            nRecvLen = recv(a_ClientSock, &RecvBuf, CRTLMSGLEN, 0);
            int64_t nFileLen;

            // 如果收到信息
            if (nRecvLen > 0) {
                // 解析信息
                switch (RecvBuf[0]) {
                    case 'N': { // Peer发来了不完整的信息
                        break;
                    }
                    case 'Q': { // Peer主动结束
                        bIsConnectionFinished = true;
                        break;
                    }
                    case 'E' : { // Peer内部出错
                        printf("[%d] There is an error\n", a_ClientSock);
                        fflush(stdout);
                        bIsConnectionFinished = true;
                        break;
                    }

                    case 'G': { // Peer想知道文件长度
                        /*
                         * G[Filename].......................\n Get file
                         * ->g[nFileLen, 8bit]...............\n
                         * ->E...............................\n
                         * B[Filename].......................\n Get block
                         * ->b[NumOfBlock, 8bit].............\n
                         * ->[IsBlockRecved]
                         * ->E...............................\n
                         * F[BlockNumber, 8bit][Filename]....\n Fetch block
                         * ->f[BlockNumber, 8Bit][BlockSize].\n
                         * ->[DATA]
                         * ->E...............................\n
                         */
                        // info: filename
                        printf("[%d] Got new file query\n", a_ClientSock);
                        string FilenameTmp;
                        FilenameTmp.assign(&RecvBuf[1]);

                        // 获取文件产固定
                        nFileLen = a_Storage.m_FileExist(FilenameTmp);
                        memset(SendBuf, 0, sizeof(char) * CRTLMSGLEN);

                        // 形成长度信息，然后发送
                        if (nFileLen >= 0) {
                            SendBuf[0] = 'g';
                            Uint64ToChar((uint64_t) nFileLen, &SendBuf[1]);
                            SendAll(a_ClientSock, SendBuf, CRTLMSGLEN);
                            printf("[%d] File: [%lu] Byte\n", a_ClientSock, nFileLen);
                        } else {
                            ReplyClientWithError(a_ClientSock);
                            printf("[%d] File doesnt exist\n", a_ClientSock);
                        }
                        break;
                    }
                    case 'B': { // Peer 想知道本Node有哪些块
                        printf("[%d] Got new block query\n", a_ClientSock);
                        string FilePathTmp;
                        FilePathTmp.assign(&RecvBuf[1]);
                        nFileLen = a_Storage.m_FileExist(FilePathTmp);

                        if (nFileLen >= 0) { // 文件块
                            // 创建并获得文件代理
                            a_Storage.m_CreateFileDelegate(FilePathTmp);
                            FileDelegate *pFDelegate = a_Storage.m_GetFileDelegatePtr(
                                    FilePathTmp);
                            if (pFDelegate != nullptr) { // 获取成功
                                // 发送Block信息，包括Block数目和状态
                                uint64_t nBlockNum = pFDelegate->NumBlocks();
                                printf("[%d] This file has [%lu] blocks\n", a_ClientSock, nBlockNum);

                                memset(SendBuf, 0, sizeof(char) * CRTLMSGLEN);
                                SendBuf[0] = 'b';
                                Uint64ToChar(nBlockNum, &SendBuf[1]);
                                SendBuf[CRTLMSGEND] = '\n';
                                SendAll(a_ClientSock, SendBuf, CRTLMSGLEN);

                                char *pBlockInfo = new char[nBlockNum];
                                BoolVecToCharArray(pFDelegate->IsBlockRecved, pBlockInfo);
                                SendAll(a_ClientSock, pBlockInfo, nBlockNum);
                                delete[] pBlockInfo;

                            } else {
                                ReplyClientWithError(a_ClientSock);
                                printf("[%d] Could not find FileDelegate\n", a_ClientSock);
                            }
                        } else {
                            ReplyClientWithError(a_ClientSock);
                            printf("[%d] File doesnt exist\n", a_ClientSock);
                        }
                        break;
                    }
                    case 'F' : { // Peer 想要Fetch一个块
                        printf("[%d] Got new block request\n", a_ClientSock);
                        uint64_t nBlockNumber = CharToUint64(&RecvBuf[1]);
                        string FilePathTmp;
                        FilePathTmp.assign(&RecvBuf[9]);
                        nFileLen = a_Storage.m_FileExist(FilePathTmp);

                        if (nFileLen >= 0) {
                            a_Storage.m_CreateFileDelegate(FilePathTmp);
                            FileDelegate *pFDelegate = a_Storage.m_GetFileDelegatePtr(FilePathTmp);
                            if (pFDelegate != nullptr) {

                                // 判断改块的状态
                                bool bIsBlockAvailable = pFDelegate->m_IsBlockAvailable(nBlockNumber);
                                printf("[%d] Block availability: %s\n", a_ClientSock,
                                       (bIsBlockAvailable ? "true" : "false"));
                                if (bIsBlockAvailable) {
                                    uint64_t nBlockLen = pFDelegate->m_GetBlockLen(nBlockNumber);

                                    // 打开文件
                                    FILE *pFIN = fopen(a_Storage.m_CompletePath(FilePathTmp).c_str(), "rb");
                                    if (pFIN != nullptr) {
                                        // 定位指针
                                        int ret = fseek(pFIN, (long) nBlockNumber * BLOCKSIZE, SEEK_SET);
                                        if (ret >= 0) {
                                            // 发送指定的长度，对应的区块号码
                                            memset(SendBuf, 0, sizeof(char) * CRTLMSGLEN);
                                            SendBuf[0] = 'f';
                                            Uint64ToChar(nBlockNumber, &SendBuf[1]);
                                            Uint64ToChar(nBlockLen, &SendBuf[9]);
                                            SendBuf[CRTLMSGEND] = '\n';
                                            printf("[%d] Block %lu Size: [%lu] Byte\n", a_ClientSock, nBlockNumber,
                                                   nBlockLen);

                                            // 发送文件的长度
                                            SendAll(a_ClientSock, SendBuf, CRTLMSGLEN);

                                            // 循环读取苏武文件
                                            SendAll(a_ClientSock, pFIN, nBlockLen);
                                        } else {
                                            ReplyClientWithError(a_ClientSock);
                                            printf("[%d] Fseek failed\n", a_ClientSock);
                                        }

                                    } else {
                                        ReplyClientWithError(a_ClientSock);
                                        printf("[%d] Could not open the file\n", a_ClientSock);
                                    }
                                } else {
                                    ReplyClientWithError(a_ClientSock);
                                    printf("[%d] Block is not available\n", a_ClientSock);
                                }
                            } else {
                                ReplyClientWithError(a_ClientSock);
                                printf("[%d] Could not find FileDelegate\n", a_ClientSock);
                            }
                        } else {
                            ReplyClientWithError(a_ClientSock);
                            printf("[%d] File doesnt exist\n", a_ClientSock);
                        }
                        break;
                    }
                    default: {
                        // 损坏的消息
                        printf("[%d] Data corrupted\n", a_ClientSock);
                    }
                }
            } else if (nRecvLen == -1) {
                printf("[%d] No incoming message\n", a_ClientSock);
                break;
            } else {
                usleep(50);
                continue;
            }
        }
    } catch (const char *err) { // 大概率是Node意外退出
        printf("[%d] Unknown error...\n", a_ClientSock);
    }

    a_MsgQueue.push(a_ClientSock);

    printf("[%d] Exitting...\n", a_ClientSock);
    fflush(stdout);
}

void NodeServeForever(int argc, char *argv[], StorageDaemon &a_Storage, string &a_TrackerHostName, int a_TrackerPort,
                      SharedPeers &a_PeerAddr) {
    /*
     * 这个函数完成两个功能
     * - 接受其他Peer的连接，创建新的线程向他们分享文件
     * - 向Tracker注册自己
     *
     * 这两个功能在同一个端口上完成
     */
    int ClientSock;
    struct sockaddr_in ClientAddr{};
    int ret;
    socklen_t nClientAddrLen;
    time_t LastRegisterTime = 0; // 上一次尝试注册
    time_t LastFailedTime = 0;  // 上一次注册失败
    vector<sockaddr_in> PeerAddrs;
    NodeConnectionDaemon ConnD;    // 启动线程管理器

    int CurrPort = argc < 2 ? NODEPORT : NODEPORT + strtol(argv[1], nullptr, 10);

    // 创建并绑定Socket到特定端口
    int SockServerFD = BindLocalTCPSocket(CurrPort, false);
    if (SockServerFD < 0) {
        printf("Socket init failed \n");
        exit(1);
    }
    cout << "Start listening at " << CurrPort << endl;

    // 开始监听
    listen(SockServerFD, 5);
    // SockServer现在是NonBlock了
    SetNonBlock(SockServerFD);

    while (true) {
        // 接受连接
        ClientSock = accept(SockServerFD, (struct sockaddr *) &ClientAddr, &nClientAddrLen);
        if (ClientSock != -1) {
            // 线程管理器释放那些已经结束的线程
            ConnD.m_Loop();
            ConnD.m_AddConnection(ClientSock, ClientAddr, a_Storage);
        } else {
            // 计算从上次注册/注册失败起的时间
            time_t Duration = (time(nullptr) - LastRegisterTime);
            time_t FailDuration = (time(nullptr) - LastFailedTime);

            // 如果注册成功了超过给定时间或者注册失败超过一定时间，重新注册
            if (Duration > (CLIENT_TIMEOUT) or
                (PeerAddrs.empty() and (FailDuration > 1))) { // more peers, less update frequency

                // 在这个函数里完成注册并获取Peer信息，函数执行完毕后SockServerFD被释放，重复两次以确保收到
                vector<sockaddr_in> NewPeerAddrs = GetPeersFromTracker(SockServerFD, a_TrackerHostName, a_TrackerPort,
                                                                       CurrPort);
                printf("Number of peers: %zu\n", NewPeerAddrs.size());

                // 判断返回的结果
                if (not NewPeerAddrs.empty()) { // 成功获取到信息
                    // 更新Peer信息
                    printf("Sucessfully get peer info\n");
                    PeerAddrs = NewPeerAddrs;
                    a_PeerAddr.m_Set(PeerAddrs);
                    time(&LastRegisterTime); // reset timer

                } else { // 未能成功获取信息
                    printf("Falied to get peer info\n");
                    time(&LastRegisterTime);
                    time(&LastFailedTime);
                }

                // 恢复监听
                SockServerFD = BindLocalTCPSocket(CurrPort, false);
                if (SockServerFD < 0) {
                    printf("Socket init failed \n");
                    exit(1);
                }
                listen(SockServerFD, 5);
                SetNonBlock(SockServerFD);

            } else {
                // 让出时间片
                this_thread::yield();
            }
        }
    }

}