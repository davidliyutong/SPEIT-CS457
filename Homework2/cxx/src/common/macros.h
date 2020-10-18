//
// Created by liyutong on 2020/10/7.
//

#ifndef P2P_FILE_SHARE_MACROS_H
#define P2P_FILE_SHARE_MACROS_H
#define SERVERPORT 18889 // Server 端口
#define TRACKERPORT  18888 // Tracker 端口
#define NODEPORT 18890 // 节点端口（UDP）

#define SENDBUFFLEN 1024 // socket一次发送的长度
#define RECVBUFFLEN 1024 // socket一次接受的长度
#define FILEBUFFLEN 40960 // 一次读取文件的长度
#define CRTLMSGLEN 512 // 节点之间通讯控制命令的长度
#define CRTLMSGEND (CRTLMSGLEN-1) //

#define BLOCKSIZE 0x100000 //1MB 一个Block
#define CHUNKSIZE 0x200 //UDP 协议，一个CHUNK 512KB
#define NCHUNK (BlockSize/CHUNKSIZE) //

#define TRACKER_TIMEOUT 0xFFFF // Tracker将Peer设为掉线的时间间隔
#define CLIENT_TIMEOUT 0x5 // CLIENT向Tracker注册的时间间隔

#define CMD_LEN 512 // 系统调用的指令长度
#define SOCKET_WAIT_LEN 5

#define SYNC_MAX_RETRY 20 // 最大尝试次数
#define SYNC_MAX_THREAD 3 // Peer 一次性同步的块数目
#define UNIX_SOCKET_PATH "/tmp/homework2"
#endif //P2P_FILE_SHARE_MACROS_H
