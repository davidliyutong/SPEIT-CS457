//
// Created by liyutong on 2020/10/5.
//
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <cstdlib>
#include "../common/StorageDaemon.h"
#include "../common/data.h"
#include "TcpNodeMain.h"

using namespace std;


void cli(queue<string> &a_FilePathQueue) {
    /*
     * 简陋的CLI界面，其实可以移植C/S模型里的，但是意义不大所以没有这么做，以后可能用HTML写个界面吧
     * 工作方式：从标准输入读取一个路径，同步该路径下的文件，注意该路径是相对RootPath的路径，而FileDelegate存放的是相对当前程序目录的路径
     */
    string command;
    while (true) {
        cout << "Enter the resource you want to sync:\n";
        cin >> command;
        a_FilePathQueue.push(command);
    }
}

int main(int argc, char *argv[]) {
    /*
     * 参数列表
     * 1[PORTDelta] 2[TRACKERHOSTNAME] 3[TRACKERPORT] 4[ROOTPATH] 5[FILES...] ...
     * PORTDelta代表Node监听端口相对NODEPORT的偏移量，这是为了在同一台Host上进行调试准备的
     * TRACKERHOSTNAME是tracker的IP地址
     * TRACKERPORT是tracker的端口
     * ROOTPATH是文件根目录
     * FILES 是以空格分割的文件
     */
    string RootPath;
    if (argc < 5) {
        RootPath.assign("./Files/");
    } else {
        RootPath.assign(argv[4]);
    }
    cout << "Root dir: " << RootPath << endl;

    // 初始化存储管理器
    StorageDaemon StorageD(RootPath);
    string TrackerHostName(argv[2]);
    int TrackerPort = strtol(argv[3], nullptr, 10);

    // PeerAddr 在线程间共享Peer信息
    SharedPeers PeerAddr;

    // 等待同步的文件队列（文件名）
    queue<string> FilePathQueue;

    // 启动一个线程向Peer分享文件
    auto *pServerThread = new thread(NodeServeForever, argc, argv, ref(StorageD), ref(TrackerHostName), TrackerPort,
                                     ref(PeerAddr));
    // 启动一个线程从Peer同步文件
    auto *pSyncThread = new thread(NodeSyncForever, ref(FilePathQueue), ref(StorageD), ref(PeerAddr));

    // 将输入参数中的文件名添加到任务队列中，这里因为队列一进一出所以不需要考虑线程安全问题
    if (argc > 5) {
        for (auto i = 5; i < argc; ++i) FilePathQueue.push(string(argv[i]));

    } else { // 参数不足，进入交互模式
        cli(FilePathQueue);
    }

    // 等待线程结束，大概永不结束
    pServerThread->join();
    pSyncThread->join();

}