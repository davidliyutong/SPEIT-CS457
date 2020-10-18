//
// Created by liyutong on 2020/9/23.
//
#include "utils.h"

void node_start() {
    std::cout << "Node started" << std::endl;
}

void node_stop() {
    std::cout << "Node stopped" << std::endl;
}

// Register tcpNode to tcpTracker. Format: share_key:node_ip:port
using namespace std;

unsigned int gen_key(const string& uuid, const string& path) {
    unsigned int key;
    string uuid_path = uuid + path;
    hash<string> h;
    key = h(uuid_path);
    return key;
}

