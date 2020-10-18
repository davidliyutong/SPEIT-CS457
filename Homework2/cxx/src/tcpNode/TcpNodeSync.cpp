//
// Created by liyutong on 2020/10/11.
//
#include <arpa/inet.h>
#include <cstdio>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>
#include <mutex>
#include <random>
#include "../common/utils.h"
#include "../common/StorageDaemon.h"
#include "../common/data.h"
#include "TcpNodeMain.h"

void SyncBlocksWithPeer(string &a_FilePath, sockaddr_in a_PeerAddr, vector<uint64_t> a_BlockNumbers,
                        FileDelegate *pFDelegate) {
    /*
     * 这个函数是接收端的子线程，用于向单个Peer建立连接传输文件块
     *
     */
    bool ret;
    bool bTransmissionPerformed;
    // 初始化一个Socket
    int Sock = GenNewTCPSock();
    while (true) {
        bTransmissionPerformed = false;
        for (auto BlockNumber : a_BlockNumbers) {
            // 该线程从文件代理处获取一个文件块的读写权限，并将该块设置为已收到
            ret = pFDelegate->m_GetBlockRecvFlag(BlockNumber);
            if (not ret) continue; // 如果失败，说明别的线程正在操作该块，故继续
            // 用这个Socket去获取文件
            FILE * pFOUT = fopen(pFDelegate->FilePath.c_str(), "rb+");
            if (pFOUT == nullptr) {
                printf("Failed to open the file when sync blocks\n");
                return;
            }

            ret = GetBlockFromRemote(Sock, a_PeerAddr, a_FilePath, BlockNumber, pFOUT);
            if (ret) {
                // 确实收到了文件块, 设置为可用
                bTransmissionPerformed = true; // 当前循环发生了传输
                pFDelegate->m_SetBlockRecvState(BlockNumber, true);
                printf("Fetch block %lu from peers\n", BlockNumber);
                fclose(pFOUT);
            } else {
                // 接受失败了，交还文件块的读写权限
                pFDelegate->m_RetBlockRecvFlag(BlockNumber);
                printf("Failed to Fetch block %lu from peers\n", BlockNumber);
                fclose(pFOUT);
            }

        }
        // 没有接受到任何文件，退出
        if (not bTransmissionPerformed) break;
    }
    fflush(stdout);
}

void SyncFileWithPeers(string &a_FilePath, SharedPeers &a_PeerAddrs, StorageDaemon &a_Storage) {
    /*
     * 并行化传输
     * 判断文件是否在磁盘上存在
     * - 文件不存在，则在内存中创建空白文件代理
     * - 文件存在，根据本地文件创建代理
     *
     * 向同伴发送请求该文件的消息
     * - 根据收到的消息（最近修改时间）判断是否向同伴发起请求
     * - 向同伴发起获得文件块的请求
     * - 收齐一个文件块，写入磁盘，更新文件代理(别的线程）
     */

    printf("Syncing %s with peers\n", a_FilePath.c_str());
    fflush(stdout);
    int Sock = GenNewTCPSock(); // 创建socket，该session共用此socket
    vector<sockaddr_in> PeerAddrs; //

    // 在内存中创建文件代理,获取文件代理的指针
    /*
     * 已知的Bug：如果文件已经被同步过一次，在内存中的文件代理将会持续存在，且会被记录为同步完成。
     * 如果外部程序删除了该文件，文件将无法再次被同步
     * 如果一个节点删除了文件，另外的节点并不会删除
     */

    int ret = a_Storage.m_CreateFileDelegate(a_FilePath);
    FileDelegate *pFDelegate = a_Storage.m_GetFileDelegatePtr(a_FilePath);

    // 准备文件代理，主要是获得文件的长度，从而在磁盘上预分配空间，这里使用轮询
    uint64_t nFileLen; // 记录文件长度
    int i = 0;
    while (not pFDelegate->bIsReady and i < SYNC_MAX_RETRY) { // 当文件代理尚未获得长度时
        PeerAddrs = a_PeerAddrs.m_Get();
        for (auto P : PeerAddrs) { // 轮询Peers
            // 获取文件长度
            ret = GetFileInfoFromRemote(P, a_FilePath, &nFileLen);
            printf("Get file info from %s:%hu\n", inet_ntoa(P.sin_addr), ntohs(P.sin_port));
            if (ret) {
                // 长度有效，更新文件代理
                printf("File len: %lu\n", nFileLen);
                pFDelegate->m_Update(nFileLen);
                break;
            }
        }
        // 没有Get到有效的文件长度，sleep一会
        printf("No valid peer addr, Peer addr len %zu\n", PeerAddrs.size());
        if (not pFDelegate->bIsReady) this_thread::sleep_for(std::chrono::milliseconds(500));
        ++i;
    }
    if (i >= SYNC_MAX_RETRY) {
        // 超过了最大尝试次数，关闭Socket 退出
        close(Sock);
        printf("File not found\n");
        fflush(stdout);
        printf("Sync failed\n");
        return;
    }


    // 接受文件并行化
    deque<thread *> deqThreads;
    thread *pThread;
    // 打开需要写入的文件（二进制）
//    FILE *pFOUT = fopen(pFDelegate->FilePath.c_str(), "rb+");
//    if (pFOUT == nullptr) {
//        // 打开文件失败
//        printf("Failed to open the file\n");
//        fflush(stdout);
//        close(Sock);
//        return;
//    }

    while (not pFDelegate->IsComplete()) {
        // 打乱Peers的顺序
        PeerAddrs = a_PeerAddrs.m_Get();
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        shuffle(PeerAddrs.begin(), PeerAddrs.end(), std::default_random_engine(seed));

        // 轮询Peers
        for (auto Target : PeerAddrs) {
            // 向Peers询问它们持有的block
            vector<bool> RemoteBlockInfo = GetBlockInfoFromRemote(Target, a_FilePath);

            // 根据返回值决定动作
            if (not RemoteBlockInfo.empty()) {
                // 如果得到了块的信息，则将得到的块信息与已有块比较
                printf("Get block info from %s:%hu\n", inet_ntoa(Target.sin_addr), ntohs(Target.sin_port));
                vector<uint64_t> TargetBlocks = CmpBlockInfo(pFDelegate->IsBlockRecved, RemoteBlockInfo, true);

                // 一次性同步SYNC_MAX_THREAD个块
                if (TargetBlocks.size() > SYNC_MAX_THREAD) {
                    TargetBlocks.assign(TargetBlocks.begin(), TargetBlocks.begin() + SYNC_MAX_THREAD);
                }
                // 如果有需要同步的块
                if (not TargetBlocks.empty()) {
                    pThread = new thread(SyncBlocksWithPeer, ref(a_FilePath), Target, TargetBlocks, pFDelegate);
                    deqThreads.push_back(pThread);

                } else {
                    // 没有需要同步的块
                    printf("Current node doesn't have the desired block\n");
                    continue;
                }
            } else {
                // 这个节点没有想要的文件
                printf("Current node doesn't have the desired file\n");
                continue;
            }
        }
        // Join threads
        while (not deqThreads.empty()) {
            pThread = deqThreads.front();
            pThread->join();
            deqThreads.pop_front();
        }
        if (not pFDelegate->IsComplete()) this_thread::sleep_for(std::chrono::milliseconds(10));

    }

    //
    printf("Sync complete\n");
    ret = a_Storage.m_RetFileDelegatePtr(&pFDelegate);
    fflush(stdout);
    close(Sock);
}

using namespace chrono;

void
NodeSyncForever(queue<string> &a_FilePathQueue, StorageDaemon &a_Storage, SharedPeers &a_PeerAddr) {
    vector<sockaddr_in> PeerAddrs;
    string sFilename;

    // 循环从任务队列中读取任务
    while (true) {
        while (not a_FilePathQueue.empty()) {
            // 读取任务，使用SyncFileWithPeers完成同步工作。这里可以实现一下并发，但是因为时间关系没有这么做
            sFilename = a_FilePathQueue.front();
            a_FilePathQueue.pop();
            auto tStart = system_clock::now();
            SyncFileWithPeers(sFilename, a_PeerAddr, a_Storage);
            auto tEnd = system_clock::now();
            auto tDuration = duration_cast<microseconds>(tEnd - tStart);
            cout << "#" << double(tDuration.count()) * microseconds::period::num / microseconds::period::den
                 << endl; // seconds
            fflush(stdout);
        }

        // 让出时间片
        this_thread::yield();
    }
}

