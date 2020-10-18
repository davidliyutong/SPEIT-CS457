//
// Created by liyutong on 2020/10/10.
//

#include "TcpClientMain.h"

using namespace std;

bool GetFileFromServer(int a_ClientSock, const string &a_ResourceLink, const string &a_Filename, bool a_Debug) {
    /*
     * 该函数的目的：
     * 通过 a_ClientSock 下载 位于a_ResourceLink的文件，保存为a_Filename
     */
    bool bIsSent = false; // 已经发出请求
    bool bIsFileRecved = false; // 已经收到文件
    bool bIsConnectionFinished = false; // 已经结束连接
    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    int nRecvLen = 0;
    int nSendLen = 0;
    const int nMaxRetry = 3; // 最大尝试次数
    int i = 0;

    // 尝试nMaxRetry次
    while (i < nMaxRetry) {
        if (not bIsSent) {
            // 生成请求文件的命令，发送
            FormulateFileRequest(a_ResourceLink, SendBuf, SENDBUFFLEN);
            nSendLen = send(a_ClientSock, SendBuf, sizeof(char) * CRTLMSGLEN, 0);
            if (nSendLen > 0) { // 发送成功
                bIsSent = true;
                if (a_Debug) printf("GET msg sent, waiting ...\n");
            } else { // 发送失败
                bIsSent = false;
                if (a_Debug) printf("Send failure\n");
            }
        }
        // 等待服务器回复
        usleep(50);
        // 将a_ClientSock 设置为非阻塞，其实这里可以用SendSockRecvTimeout()方法让socket收一段时间，然后返回
        SetNonBlock(a_ClientSock);

        // 尝试接受
        nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * CRTLMSGLEN, 0); // must set the same len as the server
        if (nRecvLen <= 0) { // 没有收到服务器的回复
            if (i < nMaxRetry) { // 重试
                bIsSent = false;
                sleep(1);
                if (a_Debug) printf("No response, retry ...\n");
                ++i;
                continue;
            }

        } else {
            // 将a_ClientSock 设置为阻塞
            SetNonBlock(a_ClientSock);
            // 根据收到的回复做出判断
            switch (RecvBuf[0]) {
                case 'Y': { // 服务器返回一个正常值
                    // 解析服务器返回的文件长度
                    uint64_t nFileLen = CharToUint64(&RecvBuf[1]);
                    // 打开文件待写
                    FILE *pFOUT;
                    pFOUT = fopen(a_Filename.c_str(), "wb");

                    if (pFOUT != nullptr) { // 成功打开文件
                        int nByteLeft = nFileLen;
                        // 循环接受所有的字节
                        while (nByteLeft > 0) {
                            if (nByteLeft >= RECVBUFFLEN) {
                                nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * RECVBUFFLEN, 0);
                            } else {
                                nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * nByteLeft, 0);
                            }
                            // 注意nRecvLen!=(设定的接收长度)，因此要根据收到的实际长度写入磁盘
                            if (nRecvLen > 0) {
                                fwrite(&RecvBuf, sizeof(char), nRecvLen, pFOUT);
                                nByteLeft -= nRecvLen;
                            }
                        }
                        // 关闭文件，结束传输
                        if (a_Debug) printf("Recving %lu byte of data\n", nFileLen - nByteLeft);
                        fclose(pFOUT);
                        bIsFileRecved = true;
                        bIsConnectionFinished = true;
                    } else { // 无法打开文件
                        if (a_Debug) printf("Permission error with file open\n");
                        bIsFileRecved = false;
                        bIsConnectionFinished = true;
                    }
                    break;
                }
                case 'E': { // 服务器返回一个异常值
                    if (a_Debug) printf("File not found\n");
                    bIsFileRecved = false;
                    bIsConnectionFinished = true;
                }
                default: { // 其他情况
                    if (a_Debug) printf("Data corrupted\n");
                    bIsFileRecved = false;
                    bIsConnectionFinished = false;
                }
            }
        }
        // 如果文件收到，那么结束循环
        if (bIsConnectionFinished) {
            break;
        } else {
            // 可尝试次数加一
            ++i;
        }

    }
    return bIsFileRecved;
}

string GetIndexFromServer(int a_ClientSock, const string &a_Path, bool a_Debug) {
    /*
     * 该函数的目的：
     * 通过 a_ClientSock 获取 位于a_ResourceLink的目录
     */
    bool bIsSent = false;
    bool bIsConnectionFinished = false;
    char SendBuf[SENDBUFFLEN];
    char RecvBuf[RECVBUFFLEN];
    int nRecvLen = 0;
    int nSendLen = 0;

    const int nMaxRetry = 3;
    int i = 0;

    string Index;

    // 尝试nMaxRetry次
    while (i < nMaxRetry) {
        if (not bIsSent) {
            // 生成请求目录的命令，发送
            FormulateIndexRequest(a_Path, SendBuf, SENDBUFFLEN);
            nSendLen = send(a_ClientSock, SendBuf, sizeof(char) * CRTLMSGLEN, 0);
            if (nSendLen > 0) { // 发送成功
                bIsSent = true;
                if (a_Debug) printf("INDEX msg sent, waiting ...\n");
            } else { // 发送失败
                bIsSent = false;
                if (a_Debug) printf("Send failure\n");
            }
        }
        // 等待服务器回复
        usleep(100);
        // 将a_ClientSock 设置为非阻塞，其实这里可以用SendSockRecvTimeout()方法让socket收一段时间，然后返回
        SetNonBlock(a_ClientSock);

        // 尝试接受
        nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * CRTLMSGLEN, 0); // must set the same len as the server
        if (nRecvLen <= 0) { // 没有收到服务器的回复
            if (i < nMaxRetry) { // 重试
                bIsSent = false;
                sleep(1);
                if (a_Debug) printf("No response, retry ...\n");
                ++i;
                continue;
            }

        } else {
            // 将a_ClientSock 设置为阻塞
            SetNonBlock(a_ClientSock);
            // 根据收到的回复做出判断
            switch (RecvBuf[0]) {
                case 'Y': { // 服务器返回一个正常值
                    // 解析服务器返回的索引长度
                    uint64_t nIndexLen = CharToUint64(&RecvBuf[1]);
                    int nByteLeft = nIndexLen;

                    // 循环接受所有的字节
                    while (nByteLeft > 0) {
                        if (nByteLeft >= RECVBUFFLEN) {
                            nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * RECVBUFFLEN, 0);
                        } else {
                            nRecvLen = recv(a_ClientSock, RecvBuf, sizeof(char) * nByteLeft, 0);
                        }
                        if (nRecvLen > 0) {
                            Index.append(RecvBuf, nRecvLen);
                            nByteLeft -= nRecvLen;
                        }
                    }
                    if (a_Debug) printf("Recving %lu byte of index\n", nIndexLen - nByteLeft);
                    bIsConnectionFinished = true;
                    break;
                }
                case 'E': { // 服务器返回一个异常值
                    if (a_Debug) printf("Dir not found\n");
                    bIsConnectionFinished = false;
                    break;
                }
                default: {
                    if (a_Debug) printf("Data corrupted\n");
                    bIsConnectionFinished = false;
                }
            }
        }

        // end if the file has been received
        if (bIsConnectionFinished) {
            break;
        } else {
            ++i;
        }

    }
    // 返回收到的目录（string）
    return Index;

}
