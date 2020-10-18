//
// Created by liyutong on 2020/10/11.
//
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <map>
#include <deque>
#include <csignal>
#include "../common/macros.h"
#include "../common/MsgInterpreter.h"
#include "../common/data.h"
#include "../common/utils.h"
#include "../common/StorageDaemon.h"
#include "../common/ConnectionDaemon.h"
#include "TcpNodeMain.h"


bool RegisterNode(int a_Sock, const string &a_RemoteHostName, int a_RemotePort, int a_CurrPort) {
    /*
     * 函数作用：注册Node
     * 函数执行后，a_Sock被关闭
     */
    int ret = 0;
    vector<sockaddr_in> PeerInfo;
    sockaddr_in TargetAddr = GenRemoteAddr(a_RemoteHostName, a_RemotePort);

    shutdown(a_Sock, SHUT_RDWR);
    close(a_Sock);
    a_Sock = BindLocalTCPSocket(a_CurrPort, false);
    ret = connect(a_Sock, (sockaddr *) &TargetAddr, sizeof(TargetAddr));
    if (ret == -1) {
        close(a_Sock);
        return false;
    }

    // 设置接收超时
    SetSockRecvTimeout(5, 0, a_Sock);

    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    // Registration...
    SendBuf[0] = 'R';
    SendBuf[CRTLMSGLEN - 1] = '\n';
    send(a_Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);
    recv(a_Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] != 'Y') {
        close(a_Sock);
        return false;
    }
    return true;
}

vector<sockaddr_in> GetPeersFromTracker(int a_Sock, const string &a_RemoteHostName, int a_RemotePort, int a_CurrPort) {
    /*
     * 该函数从Tracker接受Peer信息
     * 函数关闭a_Sock
     * 函数同时注册自己
     */
    int ret = 0;
    vector<sockaddr_in> PeerInfo;
    sockaddr_in TargetAddr = GenRemoteAddr(a_RemoteHostName, a_RemotePort);

    shutdown(a_Sock, SHUT_RDWR);
    close(a_Sock);
    a_Sock = BindLocalTCPSocket(a_CurrPort, false);
    ret = connect(a_Sock, (sockaddr *) &TargetAddr, sizeof(TargetAddr));
    if (ret == -1) {
        close(a_Sock);
        return PeerInfo;
    }

    SetSockRecvTimeout(5, 0, a_Sock);

    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    // 注册
    SendBuf[0] = 'R';
    SendBuf[CRTLMSGLEN - 1] = '\n';
    send(a_Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);
    recv(a_Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] != 'Y') {
        close(a_Sock);
        return PeerInfo;
    }
    // 获取Peer
    SendBuf[0] = 'P';
    SendBuf[CRTLMSGLEN - 1] = '\n';
    send(a_Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);
    recv(a_Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] != 'Y') {
        close(a_Sock);
        return PeerInfo;
    }

    uint64_t nPeerInfoLen = CharToUint64(&RecvBuf[1]);
    uint64_t nByteLeft = nPeerInfoLen;
    int nRecvLen;
    string sPeerInfo;

    while (nByteLeft > 0) {
        if (nByteLeft >= RECVBUFFLEN) {
            nRecvLen = recv(a_Sock, RecvBuf, sizeof(char) * RECVBUFFLEN, 0);
        } else {
            nRecvLen = recv(a_Sock, RecvBuf, sizeof(char) * nByteLeft, 0);
        }
        if (nRecvLen > 0) {
            sPeerInfo.append(RecvBuf, nRecvLen);
            nByteLeft -= nRecvLen;
        }
    }

    PeerInfo = StringToSockInfos16(sPeerInfo);
    close(a_Sock);
    return PeerInfo;
}

bool
GetBlockFromRemote(int Sock, sockaddr_in a_Target, const string &a_FilePath, uint64_t a_nBlockNumber, FILE *a_pFOUT) {
    /*
     * 该函数从远程地址获取指定的文件块
     */
    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    int ret = connect(Sock, (sockaddr *) &a_Target, sizeof(a_Target));
    if (ret < 0 and not SocketConnected(Sock)) return false;

    memset(SendBuf, 0, CRTLMSGLEN);
    SendBuf[0] = 'F';
    Uint64ToChar(a_nBlockNumber, &SendBuf[1]);
    strcpy(&SendBuf[9], a_FilePath.c_str());
    SendBuf[CRTLMSGLEN - 1] = '\n';

    send(Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);
    SetSockRecvTimeout(5, 0, Sock);

    int nRecvLen = recv(Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] == 'E' or nRecvLen < 0) return false;

    uint64_t nBlockerNumber = CharToUint64(&RecvBuf[1]);
    uint64_t nBlockLen = CharToUint64(&RecvBuf[9]);

    // 定位指针
    ret = fseek(a_pFOUT, BLOCKSIZE * nBlockerNumber, SEEK_SET);
    printf("fseek succeed, place: %lu\n", BLOCKSIZE * nBlockerNumber);
    if (ret < 0) return false;
    RecvAll(Sock, a_pFOUT, nBlockLen);

    return true;
}

vector<bool> GetBlockInfoFromRemote(sockaddr_in a_Target, const string &a_FilePath) {
    /*
     * 该函数从远端获取文件块信息
     */
    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    vector<bool> res;
    int Sock = GenNewTCPSock();
    int ret = connect(Sock, (sockaddr *) &a_Target, sizeof(a_Target));
    if (ret < 0) {
        close(Sock);
        return res;
    }

    memset(SendBuf, 0, CRTLMSGLEN);
    SendBuf[0] = 'B';
    strcpy(&SendBuf[1], a_FilePath.c_str());
    SendBuf[CRTLMSGLEN - 1] = '\n';

    send(Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);
    SetSockRecvTimeout(5, 0, Sock);

    int nRecvLen = recv(Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] == 'E' or nRecvLen <= 0) return res;
    uint64_t nBlockInfoLen = CharToUint64(&RecvBuf[1]);

    char *pBlockInfoTmp = new char[nBlockInfoLen];
    nRecvLen = recv(Sock, pBlockInfoTmp, sizeof(char) * nBlockInfoLen, 0);
    if (nRecvLen <= 0) return res;

    res.reserve(nBlockInfoLen);
    for (auto i = 0; i < nBlockInfoLen; ++i) {
        res.emplace_back((pBlockInfoTmp[i] > 0));
    }

    return res;
}

bool GetFileInfoFromRemote(sockaddr_in a_Target, const string &a_FilePath, uint64_t *a_nFileLen) {
    /*
     * 该函数从远端获取文件长度信息
     */
    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    int Sock = GenNewTCPSock();
    int ret = connect(Sock, (sockaddr *) &a_Target, sizeof(a_Target));
    if (ret < 0) {
        close(Sock);
        return false;
    }

    memset(SendBuf, 0, CRTLMSGLEN);
    SendBuf[0] = 'G';
    strcpy(&SendBuf[1], a_FilePath.c_str());
    SendBuf[CRTLMSGLEN - 1] = '\n';

    signal(SIGPIPE, SIG_IGN);
    send(Sock, &SendBuf, sizeof(char) * CRTLMSGLEN, 0);

    SetSockRecvTimeout(5, 0, Sock);

    int nRecvLen = recv(Sock, &RecvBuf, sizeof(char) * CRTLMSGLEN, 0);
    if (RecvBuf[0] == 'E' or nRecvLen <= 0) {
        close(Sock);
        return false;
    }

    *a_nFileLen = CharToUint64(&RecvBuf[1]);

    close(Sock);
    return true;
}