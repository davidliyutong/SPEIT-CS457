//
// Created by 厉宇桐 on 2020/10/4.
//

#ifndef P2P_FILE_SHARE_DATA_H
#define P2P_FILE_SHARE_DATA_H

#include <iostream>
#include <cstring>
#include "path.h"
#include <vector>
#include <mutex>

using namespace std;

struct ClientMsg {
    /* 这个结构体保存来自Client的消息
     * type 代表消息类型（G：下载，I: 查询，E:错误）
     * content 代表消息的具体内容
     * 是个没啥用的结构，只在C/S模型中出现，当时对TCP连接的收发认识不深刻，后来有更简单的实现
     */
    char type;
    string content;

    ClientMsg() {
        type = 0;
        content.clear();
    }

    ClientMsg(char a_type, string &a_content) {
        type = a_type;
        content.assign(a_content);
    }

    explicit ClientMsg(char a_type) {
        type = a_type;
        content.clear();
    }
};

struct PeerInfo {
    string IP;
    int Port;

    PeerInfo(string a_IP, int a_Port) {
        IP = std::move(a_IP);
        Port = a_Port;
    }
};

class FileIndex {
    /*
     * 这个Class意图保存本地文件的索引（树状）：
     * 当客户端连接到服务器时，服务器返回整个目录下的所有文件的名称、目录结构...客户端随即决定下载哪些文件。
     * 但是后来没有选用这个路线，因为这样会传输额外的数据，并且实现起来也很麻烦。
     * 实际计划在客户端实现目录的构建（并未实现）
     * 实际实现的是客户端向服务器发出列出指定目录的需求，服务器发送指定目录的文件
     */
public:
    string sRootPath;
    vector<FileIndex> vecSubDir;
    vector<string> vecSubFile;

    explicit FileIndex(string &a_Path) {
        sRootPath = a_Path;
        vecSubFile = ListFileExt(a_Path);
        vector<string> vecSubDirStr = ListDirExt(a_Path);
        for (auto it:vecSubDirStr) {
            vecSubDir.emplace_back(it);
        }
    }
};

class SharedPeers{
    /*
     * 这个结构用于在（可能）的多个线程间同步Peer信息。
     * P2P模式下，单个节点有两个线程活动（Server，Sync）
     * Server线程负责向Tracker注册（使用Server的监听端口），得到Peers数据
     * Sync根据得到的数据与Peer通讯
     */
public:
    vector<sockaddr_in> m_Get(){
        return PeerAddr;
    };
    bool m_Set(vector<sockaddr_in> &a_Val) {
        // 线程同步
        this->lockPeerAddr.lock();
        this->PeerAddr.assign(a_Val.begin(), a_Val.end());
        this->lockPeerAddr.unlock();
        return true;
    };
private:
    vector<sockaddr_in> PeerAddr{};
    mutex lockPeerAddr{};
};

#endif //P2P_FILE_SHARE_DATA_H
