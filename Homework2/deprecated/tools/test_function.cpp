//
// Created by liyutong on 2020/9/29.
//
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include "../../src/tcpTracker/PeerDaemon.h"
#include "../../src/common/macros.h"
#include "../../src/common/utils.h"
#include <unistd.h>

using namespace std;


int main() {
    sockaddr_in SockInfo = GenRemoteAddr("127.0.0.1", 18889);
    sockaddr a_SockInfo{};
    memcpy(&a_SockInfo, &SockInfo, sizeof(SockInfo));

    vector<char> vecSockInfo;
    vecSockInfo.reserve(sizeof(a_SockInfo));
    for (int i = 0; i < sizeof(a_SockInfo); i++) {
        vecSockInfo.push_back(((char *) &a_SockInfo)[i]);
    }

    string sSockInfo;
    sSockInfo.assign((const char *) &a_SockInfo, sizeof(a_SockInfo));

    return 0;
}