//
// Created by liyutong on 2020/10/5.
//

#ifndef P2P_FILE_SHARE_TCPCLIENTMAIN_H
#define P2P_FILE_SHARE_TCPCLIENTMAIN_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <cstdio>
#include "../common/utils.h"
#include "../common/macros.h"
#include <sys/un.h>

using namespace std;

bool GetFileFromServer(int a_ClientSock, const string &a_ResourceLink, const string &a_Filename, bool a_Debug);

string GetIndexFromServer(int a_ClientSock, const string &a_Path, bool a_Debug);

void cli();


#endif //P2P_FILE_SHARE_TCPSERVERMAIN_H
