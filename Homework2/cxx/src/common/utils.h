//
// Created by 厉宇桐 on 2020/10/4.
//

#ifndef P2P_FILE_SHARE_UTILS_H
#define P2P_FILE_SHARE_UTILS_H

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

using namespace std;

// 数值操作，类型转换

uint64_t CharToUint64(char *a_src);

uint32_t CharToUint32(char *a_Src);

void Uint64ToChar(uint64_t a_Src, char *a_Dst);

sockaddr_in Char16ToSockInfo(char *a_pCharArray);

bool SockInfoToChar16(const sockaddr_in &a_Addr, char *a_Dst);

vector<uint64_t> CmpBlockInfo(const vector<bool> &a_Local, const vector<bool> &a_Remote, bool a_Shuffle);

vector<sockaddr_in> StringToSockInfos16(const string &a_sChar);

bool BoolVecToCharArray(vector<bool> a_Src, char *Dst);

// 创建Socket 操作

struct sockaddr_in GenSockAddr(string &address, int port);

int GenNewTCPSock();

void SetNonBlock(int a_Sock);

void SetBlock(int a_Sock);

int BindLocalTCPSocket(int a_Port, bool a_bNonBlock);

int BindLocalUDPSocket(int a_Port, bool a_bNonBlock);


// Send Recv 操作（具体传输）

bool SendAll(int &a_Sock, char *a_SendBuffer, int a_nTotalLen);

bool SendAll(int &a_Sock, FILE *pFIN, int a_nTotalLen);

bool RecvAll(int &a_Sock, char *a_RecvBuffer, int a_nTotalLen);

char *RecvAll(int &a_Sock, int a_nTotalLen);

bool RecvAll(int &a_Sock, FILE *pFOUT, int a_nTotalLen);

//bool RecvAll(int &a_Sock, int a_FD, int a_nTotalLen);

// 控制命令有关操作

int ReplyClientWithError(int a_ClientSock);

int FormulateFileRequest(const string &FilePath, char *a_SendBuf, int a_nSendBufLen);

int FormulateIndexRequest(const string &Path, char *a_SendBuf, int a_nSendBufLen);

int FormulateFileLenResp(int nFileLen, char *a_SendBuf, int a_nSendBufLen);

struct sockaddr_in GenRemoteAddr(const string &a_Hostname, int a_Port);

// 配置Socket操作

int SetSockSendTimeout(int a_t_sec, int a_t_usec, int a_sock);

int SetSockRecvTimeout(int a_t_sec, int a_t_usec, int a_sock);

bool SocketConnected(int sock);

// 文件和目录操作

int MakeDirs(string &DirPath);

vector<string> SpliteDirFile(string &FilePath);

#endif //P2P_FILE_SHARE_UTILS_H