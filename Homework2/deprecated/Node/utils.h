//
// Created by liyutong on 2020/9/23.
//
#include <iostream>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<iostream>
#include<vector>
#include<queue>

#ifndef P2P_FILE_SHARE_UTILS_H
#define P2P_FILE_SHARE_UTILS_H

void node_start();
void node_stop();
bool register_node(std::queue<std::string> send_queue_async, const std::vector<std::string> &share_keys);
bool query_key(std::queue<std::string> send_queue_sync, const std::vector<std::string> &keys);
unsigned int gen_key(const std::string& uuid, const std::string& path);

#endif //P2P_FILE_SHARE_UTILS_H
