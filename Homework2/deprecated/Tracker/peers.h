//
// Created by liyutong on 2020/9/28.
//

#ifndef P2P_FILE_SHARE_PEERS_H
#define P2P_FILE_SHARE_PEERS_H

#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include "main.h"

using namespace std;

class peerTable {
public:
    map<string, sockaddr_in> queryEntry(const string &ruid);

    bool insertEntry(const string &ruid, const string &hostname, const struct sockaddr_in &socket_info);

    bool deleteEntry(const string &hostname);

private:
    map<string, map<string, sockaddr_in>> data;
};

map<string, sockaddr_in> peerTable::queryEntry(const string &ruid) {
    map<string, sockaddr_in> res;
    auto it = data.find(ruid);
    if (it != data.end()) {
        res.swap(it->second);
    }
    return res;
}

bool peerTable::insertEntry(const string &ruid, const string &hostname, const sockaddr_in &socket_info) {
    auto it = data.find(ruid);
    if (it != data.end()) {
        auto it2 = it->second.find(hostname);
        if (it2 != it->second.end()) {
            return false;
        } else {
            it->second[hostname] = socket_info;
        }
    } else {
        map<string, sockaddr_in> new_entry;
        new_entry[hostname] = socket_info;
        data[ruid] = new_entry;
    }
    return true;
}

bool peerTable::deleteEntry(const string &hostname) {
    // when a peer is offline for a while, delete it
    for (auto & it : data) {
        auto it2 = it.second.find(hostname);
        if (it2 != it.second.end()) {
            it.second.erase(it2);
        }
    }
    return false;
}


#endif //P2P_FILE_SHARE_PEERS_H
