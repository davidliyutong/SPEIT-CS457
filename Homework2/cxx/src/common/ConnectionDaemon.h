//
// Created by liyutong on 2020/10/11.
//

#ifndef P2P_FILE_SHARE_CONNECTIONDAEMON_H
#define P2P_FILE_SHARE_CONNECTIONDAEMON_H
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <unistd.h>

using namespace std;

class ConnectionDaemon {
    /*
     * 这个类管理了这socket连接和通讯线程
     */
public:
    virtual void m_AddConnection(){};
    void m_Loop(){
        KillOnSignal();
    }

protected:
    map<int, thread *> mapActiveClients{};
    queue<int> qMsgQueue;
    mutex lockMsgQueue{};

    void KillOnSignal(){
        /*
         * 根据MsgQueue的消息决定是否将线程Join
         */
        lockMsgQueue.lock();
        while (not qMsgQueue.empty()) {
            int curr_sock = qMsgQueue.front();
            auto it = mapActiveClients.find(curr_sock);
            if (it != mapActiveClients.end()) {
                it->second->join();
                free(it->second);
                qMsgQueue.pop();
                printf("Killed process: %d\n", curr_sock);
                // Join之后关闭Socket
                close(curr_sock);
                fflush(stdout);
                // 删除记录
                mapActiveClients.erase(it);
            }
        }
        lockMsgQueue.unlock();
    }
};


#endif //P2P_FILE_SHARE_CONNECTIONDAEMON_H
