//
// Created by liyutong on 2020/10/6.
//

#ifndef P2P_FILE_SHARE_PEERDAEMON_H
#define P2P_FILE_SHARE_PEERDAEMON_H

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "../common/macros.h"
#include "../common/data.h"

using namespace std;


class PeerDaemon {
    /*
     * 这个类是Tracker用于记录Peers信息的类
     */
public:
    bool m_AddPeer(sockaddr &a_SockInfo) {
        /*
         * 这个函数将一个sockaddr格式的peer信息保存下来，返回是否成功
         * 同一个PeerDaemon可能会被Tracker的多个连接线程访问，因此需要线程同步
         */
        lockWriteLock.lock();
        // 强制类型转换sockaddr->sockaddr_in
        sockaddr_in SockInfo{};
        memcpy(&SockInfo, &a_SockInfo, sizeof(a_SockInfo));

        // 将SockInfo的内存值直接转换成字符串，作为map的key，
        string sSockInfo;
        sSockInfo.assign((const char *) &a_SockInfo, sizeof(a_SockInfo));

        // 向map中添加key-value 键值对，这里是覆盖式添加
        Peers[sSockInfo] = SockInfo;

        // 更新这个Peer的注册时间，用于判断该Peer是否掉线
        time_t CurrTime;
        time(&CurrTime);
        PeersLastRegTime[sSockInfo] = CurrTime;
        lockWriteLock.unlock();
        return true;
    }

    string m_QueryPeer(sockaddr a_SockInfo) {
        /*
         * 这个函数根据输入的sockaddr，查找出它的Peers（不含其自身）
         * 函数的返回值是string，这个string以16字为一组存储sockaddr信息
         */
        vector<sockaddr_in> SockInfoOthers;
        string res;

        // a_SockInfo 转换成string
        string sSockInfo;
        sSockInfo.assign((const char *) &a_SockInfo, sizeof(a_SockInfo));

        // 如果找不到该Node，添加为新peer
        auto it = Peers.find(sSockInfo);
        if (it == Peers.end()) {
            m_AddPeer(a_SockInfo);
        }

        // 将该Node的Peer找出来, 合成字符串
        string CurrPeer;
        for (it = Peers.begin(); it != Peers.end(); ++it) {
            if (it->first != sSockInfo) {
                CurrPeer.assign((const char *) &it->second, sizeof(it->second));
                res += CurrPeer;
            }
        }

        return res;
    }

    int m_AutoRemovePeers() {
        /*
         * 该函数用于将掉线的Peers剔除，判断的依据是长期没有注册
         */
        lockWriteLock.lock();
        int nPeersRemoved = 0;
        time_t CurrTime;
        time(&CurrTime);

        for (auto it = PeersLastRegTime.begin(); it != PeersLastRegTime.end(); ++it) {
            if (CurrTime - it->second > TRACKER_TIMEOUT) {
                PeersLastRegTime.erase(it);
                auto it2 = Peers.find(it->first);
                if (it2 != Peers.end()) Peers.erase(it2);
                nPeersRemoved += 1;
            }
        }

        lockWriteLock.unlock();
        return nPeersRemoved;
    }

    void m_Loop() {
        /*
         * 维护信息
         */
        m_AutoRemovePeers();
    }

private:
    map<string, sockaddr_in> Peers;
    map<string, time_t> PeersLastRegTime;
    mutex lockWriteLock;
};

#endif //P2P_FILE_SHARE_PEERDAEMON_H
