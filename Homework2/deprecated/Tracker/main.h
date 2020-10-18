//
// Created by liyutong on 2020/9/23.
//
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#ifndef P2P_FILE_SHARE_MAIN_H
#define P2P_FILE_SHARE_MAIN_H

using namespace std;
struct socketMsg{
    string str;
    sockaddr_in socket_info;
};

struct peerEntry {
    string ruid;
    string hostname;
    sockaddr_in socket_info;
    peerEntry(const string &input_ruid, const string &input_hostname, const sockaddr_in &input_socket_info) {
        this->ruid.assign(input_ruid);
        this->hostname.assign(input_hostname);
        memcpy(this->socket_info, input_socket_info, sizeof(input_socket_info));
    }
};
void tracker_start() {
    std::cout << "Tracker started" << std::endl;
}

void tracker_stop() {
    std::cout << "Tracker stopped" << std::endl;
}


#endif //P2P_FILE_SHARE_MAIN_H
