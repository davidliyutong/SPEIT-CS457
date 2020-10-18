//
// Created by liyutong on 2020/10/5. 头文件
//

#ifndef P2P_FILE_SHARE_TCPNODEMAIN_H
#define P2P_FILE_SHARE_TCPNODEMAIN_H

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

using namespace std;

void NodeServeForever(int argc, char *argv[], StorageDaemon &a_Storage, string &a_TrackerHostName, int a_TrackerPort,
                      SharedPeers &a_PeerAddr);

void
NodeSyncForever(queue<string> &a_FilePathQueue, StorageDaemon &a_Storage, SharedPeers &a_PeerAddr);

bool RegisterNode(int a_Sock, const string &a_RemoteHostName, int a_RemotePort, int a_CurrPort);

vector<sockaddr_in> GetPeersFromTracker(int a_Sock, const string &a_RemoteHostName, int a_RemotePort, int a_CurrPort);

bool
GetBlockFromRemote(int Sock, sockaddr_in a_Target, const string &a_FilePath, uint64_t a_nBlockNumber, FILE *a_pFOUT);

vector<bool> GetBlockInfoFromRemote(sockaddr_in a_Target, const string &a_FilePath);

bool GetFileInfoFromRemote(sockaddr_in a_Target, const string &a_FilePath, uint64_t *a_nFileLen);

void
HandleConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, queue<int> &a_MsgQueue, StorageDaemon &a_Storage);

void SyncBlocksWithPeer(string &a_FilePath, sockaddr_in a_PeerAddr, vector<uint64_t> a_BlockNumbers,
                        FileDelegate *pFDelegate);

void SyncFileWithPeers(string &a_FilePath, SharedPeers &a_PeerAddrs, StorageDaemon &a_Storage);


class NodeConnectionDaemon : public ConnectionDaemon {
public:
    void m_AddConnection(int a_ClientSock, struct sockaddr_in a_ClientAddr, StorageDaemon &a_Storage) {
        auto *pThread = new thread(HandleConnection, a_ClientSock, a_ClientAddr, ref(qMsgQueue), ref(a_Storage));
        mapActiveClients[a_ClientSock] = pThread;

    }
};

#endif //P2P_FILE_SHARE_TCPSERVERMAIN_H
