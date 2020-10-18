//
// Created by liyutong on 2020/9/29.
//


#include "TcpClientMain.h"

using namespace std;
using namespace chrono;


int main(int argc, char *argv[]) {
    if (argc > 1) {
        // usage: tcpServer [ip] [port] [remote file] [local file]
        // example: tcpServer 127.0.0.1 18889 test1/i.doc i.doc
        const string sRemoteHostname(argv[1]);
        const int RemotePort = stoi(argv[2]);
        const string ResourceLink(argv[3]);
        const string Filename(argv[4]);
        bool bDebug = true;
        auto tStart = system_clock::now();

        // 生成用于连接的地址
        sockaddr_in RemoteAddr = GenRemoteAddr(sRemoteHostname, RemotePort);
        // 创建Socket
        int ClientSock = GenNewTCPSock();
        // 连接
        int ret = connect(ClientSock, (struct sockaddr *) &RemoteAddr, sizeof(RemoteAddr));
        if (ret < 0) {
            printf("Connecting to %s:%d failed due to port and ip problems\n", sRemoteHostname.c_str(), RemotePort);
            fflush(stdout);
            return -1;
        }

        // 从服务器下载文件
        bool res = GetFileFromServer(ClientSock, ResourceLink, Filename, bDebug);
        if (res) {
            cout << "File saved" << endl;
            auto tEnd   = system_clock::now();
            auto tDuration = duration_cast<microseconds>(tEnd - tStart);
            cout <<  double(tDuration.count()) * microseconds::period::num / microseconds::period::den << endl;// seconds
            fflush(stdout);
        } else {
            cout << "File not saved" << endl;
            fflush(stdout);
        }
    } else {
        cli();
    }
    fflush(stdout);
    return 0;
}
