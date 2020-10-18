//
// Created by liyutong on 2020/10/11.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include "../common/macros.h"
#include "utils.h"

using namespace std;

uint64_t CharToUint64(char *a_src) { //不考虑字节序
    uint64_t dst;
    std::memcpy(&dst, a_src, sizeof(char) * 8);
    return dst;
}

uint32_t CharToUint32(char *a_Src) { //不考虑字节序
    uint64_t dst;
    std::memcpy(&dst, a_Src, sizeof(char) * 4);
    return dst;
}

void Uint64ToChar(uint64_t a_Src, char *a_Dst) {
    std::memcpy(a_Dst, &a_Src, sizeof(char) * 8);
}

sockaddr_in Char16ToSockInfo(char *a_pCharArray) {
    sockaddr_in res{};
    memcpy(&res, a_pCharArray, sizeof(char) * 16);
    return res;
}

bool SockInfoToChar16(const sockaddr_in &a_Addr, char *a_Dst) {
    memcpy(a_Dst, (char *) (&a_Addr), sizeof(char) * 16);
    return true;
}

vector<uint64_t> CmpBlockInfo(const vector<bool> &a_Local, const vector<bool> &a_Remote, bool a_Shuffle) {
    vector<uint64_t> res;
    if (a_Local.size() != a_Remote.size()) return res;
    for (auto i = 0; i < a_Local.size(); i++) {
        if (a_Remote[i] and (not a_Local[i])) {
            res.push_back(i);
        }
    }
    if (a_Shuffle) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        shuffle(res.begin(), res.end(), std::default_random_engine(seed));
    }
    return res;
}

vector<sockaddr_in> StringToSockInfos16(const string &a_sChar) {
    vector<sockaddr_in> res;
    sockaddr_in SockInfoTmp{};
    if (a_sChar.length() % 16 != 0) return res;
    const char *pChar = a_sChar.c_str();

    for (auto i = 0; i < a_sChar.length() / 16; ++i) {
        memcpy(&SockInfoTmp, &pChar[i * 16], sizeof(char) * 16);
        res.push_back(SockInfoTmp);
    }
    return res;
}

bool BoolVecToCharArray(vector<bool> a_Src, char *Dst) {
    int loc = 0;
    for (auto it : a_Src) Dst[loc++] = it;
}

struct sockaddr_in GenSockAddr(string &address, int port) {
    struct sockaddr_in SockInfo{};
    SockInfo.sin_family = AF_INET;
    SockInfo.sin_port = port;
    if (inet_aton(address.c_str(), &SockInfo.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    return SockInfo;
}


int GenNewTCPSock() {
    int Sock = 0;
    Sock = socket(AF_INET, SOCK_STREAM, 0);

    if (Sock < 0) {
        printf("Socket creation failed \n");
        return -1;
    } else {
        struct timeval timeout = {1, 0};
        int ret = setsockopt(Sock, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
        ret = setsockopt(Sock, SOL_SOCKET, SO_KEEPALIVE, (const int *) 1, sizeof(int));
        return Sock;
    }
}

void SetNonBlock(int a_Sock) {
    int flags = fcntl(a_Sock, F_GETFL, 0);
    fcntl(a_Sock, F_SETFL, flags | O_NONBLOCK);
}

void SetBlock(int a_Sock) {
    int flags = fcntl(a_Sock, F_GETFL, 0);
    fcntl(a_Sock, F_SETFL, flags & ~O_NONBLOCK);
}

int BindLocalTCPSocket(int a_Port, bool a_bNonBlock) {
    const int nRetry = 3;
    int SockFD = -1;
    int i = 0;
    while (i < nRetry) {
        ++i;
        struct sockaddr_in SockInfo{};

        if ((SockFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            continue;
        }

        if (a_bNonBlock) {
            SetNonBlock(SockFD);
        } else {
            SetBlock(SockFD);
        }

        int opt = 1;
        setsockopt(SockFD, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt));

        memset((char *) &SockInfo, 0, sizeof(SockInfo));
        SockInfo.sin_family = AF_INET;
        SockInfo.sin_port = htons(a_Port);
        SockInfo.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(SockFD, (struct sockaddr *) &SockInfo, sizeof(SockInfo)) == -1) {
            printf("Bind failed\n");
            fflush(stdout);
            continue;
        } else {
            return SockFD;
        }
    }
    return -1;
}

int BindLocalUDPSocket(int a_Port, bool a_bNonBlock) {
    const int nRetry = 3;
    int Sock = -1;
    int i = 0;
    while (i < nRetry) {
        ++i;
        struct sockaddr_in SockInfo{};

        if ((Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            continue;
        }

        if (a_bNonBlock) {
            SetNonBlock(Sock);
        } else {
            SetBlock(Sock);
        }

        memset((char *) &SockInfo, 0, sizeof(SockInfo));
        SockInfo.sin_family = AF_INET;
        SockInfo.sin_port = htons(a_Port);
        SockInfo.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(Sock, (struct sockaddr *) &SockInfo, sizeof(SockInfo)) == -1) {
            printf("Bind failed\n");
            fflush(stdout);
            continue;
        } else {
            return Sock;
        }
    }
    return -1;
}


bool SendAll(int &a_Sock, char *a_SendBuffer, int a_nTotalLen) {
    int nSendLen = 0;
    while (a_nTotalLen > 0) {
        nSendLen = send(a_Sock, a_SendBuffer, a_nTotalLen, 0);
        if (nSendLen == -1)
            return false;
        a_nTotalLen = a_nTotalLen - nSendLen;//用于循环发送且退出功能
        a_SendBuffer += nSendLen;//用于计算已发buffer的偏移量
    }
    return true;
}


bool SendAll(int &a_Sock, FILE *pFIN, int a_nTotalLen) {
    if (pFIN != nullptr) { // successfully opened the file
        char FileBuf[FILEBUFFLEN];
        //Read file and send
        int nByteLeft = a_nTotalLen;
        int nSendLen;
        int nReadLen = 0;
        while (nByteLeft > 0) {
            if (nByteLeft >= FILEBUFFLEN) {
                nReadLen += fread(&FileBuf, sizeof(char), FILEBUFFLEN, pFIN);
                nSendLen = send(a_Sock, &FileBuf, FILEBUFFLEN, 0);
            } else {
                nReadLen += fread(&FileBuf, sizeof(char), nByteLeft, pFIN);
                nSendLen = send(a_Sock, &FileBuf, nByteLeft, 0);
            }
            nByteLeft -= nSendLen;
        }
        printf("[%d] Reading %d byte of data from disk\n", a_Sock, nReadLen);
        printf("[%d] Sending %d byte of data\n", a_Sock, a_nTotalLen - nByteLeft);
        fflush(stdout);
        return true;
    } else { // failed to open the file
        return false;
    }
}

bool RecvAll(int &a_Sock, char *a_RecvBuffer, int a_nTotalLen) {
    int nRecvLen = 0;
    while (a_nTotalLen > 0)//剩余部分大于0
    {
        nRecvLen = recv(a_Sock, a_RecvBuffer, a_nTotalLen, 0);
        if (nRecvLen == -1)
            return false;
        a_nTotalLen = a_nTotalLen - nRecvLen;
        a_RecvBuffer += nRecvLen;
    }
    return true;
}

char *RecvAll(int &a_Sock, int a_nTotalLen) {
    char *Dst = new char[a_nTotalLen];
    char *p = Dst;
    int nRecvLen = 0;
    while (a_nTotalLen > 0)//剩余部分大于0
    {
        nRecvLen = recv(a_Sock, p, a_nTotalLen, 0);
        if (nRecvLen == -1) {
            delete[] Dst;
            return nullptr;
        }
        a_nTotalLen = a_nTotalLen - nRecvLen;
        p += nRecvLen;
    }
    return Dst;
}

bool RecvAll(int &a_Sock, FILE *pFOUT, int a_nTotalLen) {
    if (pFOUT != nullptr) { // successfully opened the file
        char FileBuf[FILEBUFFLEN];
        //Read file and send
        int nByteLeft = a_nTotalLen;
        int nRecvLen;
        int nWriteLen = 0;
        while (nByteLeft > 0) {
            if (nByteLeft >= FILEBUFFLEN) {
                nRecvLen = recv(a_Sock, &FileBuf, FILEBUFFLEN, 0);
            } else {
                nRecvLen = recv(a_Sock, &FileBuf, nByteLeft, 0);
            }
            if (nRecvLen > 0) {
                nWriteLen += fwrite(&FileBuf, sizeof(char), nRecvLen, pFOUT);
                nByteLeft -= nRecvLen;
            } else {
                printf("[%d] Problems encountered\n", a_Sock);
            }
        }
        printf("[%d] Receiving %d byte of data\n", a_Sock, a_nTotalLen - nByteLeft);
        printf("[%d] Writting %d byte of data to disk\n", a_Sock, nWriteLen);
        fflush(pFOUT);
        return true;
    } else { // failed to open the file
        return false;
    }
}

//bool RecvAll(int &a_Sock, int a_FD, int a_nTotalLen) {
//    if (a_FD >= 0) { // successfully opened the file
//        char FileBuf[FILEBUFFLEN];
//        //Read file and send
//        int nByteLeft = a_nTotalLen;
//        int nRecvLen;
//        while (nByteLeft > 0) {
//            if (nByteLeft >= FILEBUFFLEN) {
//                nRecvLen = recv(a_Sock, &FileBuf, FILEBUFFLEN, 0);
//                write(a_FD, &FileBuf, sizeof(char) * FILEBUFFLEN);
//            } else {
//                nRecvLen = recv(a_Sock, &FileBuf, nByteLeft, 0);
//                write(a_FD, &FileBuf, sizeof(char) * nByteLeft);
//            }
//            nByteLeft -= nRecvLen;
//        }
//        printf("[%d] Receiving %d byte of data\n", a_Sock, a_nTotalLen - nByteLeft);
//        fflush(stdout);
//        return true;
//    } else { // failed to open the file
//        return false;
//    }
//}

int ReplyClientWithError(int a_ClientSock) {
    char SendBuf[SENDBUFFLEN];
    memset(SendBuf, 0, sizeof(char) * SENDBUFFLEN);
    SendBuf[0] = 'E';
    SendBuf[1] = '\n';
    int nSendLen = send(a_ClientSock, &SendBuf, CRTLMSGLEN, 0);
    return nSendLen;
}


int FormulateFileRequest(const string &FilePath, char *a_SendBuf, const int a_nSendBufLen) {
    if (FilePath.length() <= a_nSendBufLen) {
        memset(&a_SendBuf[0], 0, sizeof(char) * a_nSendBufLen);
        a_SendBuf[0] = 'G';
        memcpy(&a_SendBuf[1], FilePath.c_str(), FilePath.length());
        a_SendBuf[FilePath.length() + 1] = '\n';
        return strlen(a_SendBuf);
    } else {
        return -1;
    }
}

int FormulateIndexRequest(const string &Path, char *a_SendBuf, const int a_nSendBufLen) {
    if (Path.length() <= a_nSendBufLen) {
        memset(&a_SendBuf[0], 0, sizeof(char) * a_nSendBufLen);
        a_SendBuf[0] = 'I';
        memcpy(&a_SendBuf[1], Path.c_str(), Path.length());
        a_SendBuf[Path.length() + 1] = '\n';
        return strlen(a_SendBuf);
    } else {
        return -1;
    }
}

int FormulateFileLenResp(int nFileLen, char *a_SendBuf, const int a_nSendBufLen) {
    memset(&a_SendBuf[0], 0, sizeof(char) * a_nSendBufLen);
    a_SendBuf[0] = 'Y';
    Uint64ToChar(nFileLen, &a_SendBuf[1]);
    return 9;
}

struct sockaddr_in GenRemoteAddr(const string &a_Hostname, const int a_Port) {
    struct sockaddr_in RemoteAddr{};
    RemoteAddr.sin_family = AF_INET;
    RemoteAddr.sin_port = htons(a_Port);
    RemoteAddr.sin_addr.s_addr = inet_addr(a_Hostname.c_str());
    return RemoteAddr;
}

int SetSockSendTimeout(int a_t_sec, int a_t_usec, int a_sock) {
    struct timeval timeout = {a_t_sec, a_t_usec};
    int ret = setsockopt(a_sock, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
    if (ret != 0) {
        printf("Sned timeout unset\n");
        fflush(stdout);
    }
    return ret;
}

int SetSockRecvTimeout(int a_t_sec, int a_t_usec, int a_sock) {
    struct timeval timeout = {a_t_sec, a_t_usec};
    int ret = setsockopt(a_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    if (ret != 0) {
        printf("Recv timeout unset\n");
        fflush(stdout);
    }
    return ret;
}

bool SocketConnected(int sock) {
    if (sock <= 0) return false;
    struct tcp_info info{};
    int len = sizeof(info);
    getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *) &len);
    if (info.tcpi_state == TCP_ESTABLISHED) {
        return true;
    } else {
        return false;
    }
}

int MakeDirs(string &DirPath) {
    if (!access(DirPath.c_str(), F_OK)) {                        /* 判断目标文件夹是否存在 */
        return 1;
    }

    char *cmd = new char[CMD_LEN];
    pid_t result;

    memset(cmd, 0, CMD_LEN);
    sprintf(cmd, "mkdir -p %s", DirPath.c_str());
    result = system(cmd);

    delete[] cmd;

    if (-1 == result || !WIFEXITED(result) || 0 != WEXITSTATUS(result)) {
        return 2;
    }
    return 1;
}

vector<string> SpliteDirFile(string &FilePath) {
    string tmp;
    vector<string> res;
    res.reserve(2);
    int index = FilePath.rfind('/');
    if (index < FilePath.length()) {
        tmp.assign(FilePath, 0, index);
        res.push_back(tmp);
        tmp.assign(FilePath, index + 1);
        res.push_back(tmp);
    }
    return res;
}