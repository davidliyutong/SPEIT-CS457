//
// Created by 厉宇桐 on 2020/9/24.
//
#include <cstring>
#include <iostream>
#include <vector>
#include <queue>
#include <uuid/uuid.h>

using namespace std;

struct node_entry {
    unsigned int ruid = 0;
    string addr;
    int port = 0;
};

/*
 * Register socket_in on the tcpTracker by socket_in [sync]
 * Get peer information (other's socket_in) by socket_out [sync]
 * Tell peers my socket_in by socket_out [sync]
 * Ask peers to send their index by socket_out [sync]
 * Recv index by socket_in [async]
 * (Compare index)
 * Ask peers to send their file by socket_out [sync]
 * Recv file by socket_in [async]
 *
 * Send index by socket_out [sync]
 * Send file by socket_out   [sync]
*/

/*
 * Exchanged info:
 *
 * Register: "[ruid]\nRE\n\n"
 *
 * Get peer information: "[ruid]\nRP\n\n"
 * -> Send peer information "[ruid]\nSP\n[status]\n[addr]\n[port]\n\n"
 * -- The tcpTracker will send "[ruid]\nSP\n0\n[my_addr]\n[my_port]" first
 *
 * Ask index: "[ruid]\nRI\n[my_addr]\n[my_port]\n\n"
 * -> Send index: "[ruid]\nSI\n[filename]\n[datetime]\n\n"
 *
 * Ask file: "[ruid]\nRF\n[filename]\n\n"
 * -> Send file: "[ruid]\nSF\n[payload]\n\n"

*/

class connection_daemon {
public:

    struct node_entry tracker;
    struct node_entry host_in;
    struct node_entry host_out;
    vector<struct node_entry> peers;
    vector<string> ruids;

    unsigned int last_update_time;
    int status;

    void loop();

    static bool register_node(queue<string> send_queue_async, const vector<string> &ruids);

    connection_daemon(const struct node_entry &h_in, const struct node_entry &h_out, const struct node_entry &t) {
        last_update_time = 0;
        status = 0;
        host_in = h_in;
        host_out = h_out;
        tracker = t;
    }

    ~connection_daemon() = default;
};


void connection_daemon::loop() {
    return;
}

bool connection_daemon::register_node(queue<string> send_queue_async, const vector<string> &ruids) {
    string buf;
    string REGISTER_HEADER = "RE";
    for (const auto &it : ruids) {
        buf.assign(it);
        buf += "\n";
        buf += REGISTER_HEADER;
        buf += "\n\n";
        try {
            send_queue_async.push(buf);
        } catch (const char *err) {
            fprintf(stderr, "%s", err);
            return false;
        }
    }

    return true;
}

//// Request peers from tcpTracker
//bool request_peer(queue<string> send_queue_async, const vector<std::string> &ruids) {
//    string buf;
//    string REQUEST_PEER_HEADER = "RP";
//    for (const auto &it : ruids) {
//        buf.assign(it);
//        buf += "\n";
//        buf += REQUEST_PEER_HEADER;
//        buf += "\n\n";
//        try {
//            send_queue_async.push(buf);
//        } catch (const char *err) {
//            fprintf(stderr, "%s", err);
//            return false;
//        }
//    }
//
//    return true;
//}
//
////Ask index: "[ruid]\nRI\n[my_addr]\n[my_port]\n\n"
//
//bool ask_index(queue<string> send_queue_async, const vector<std::string> &ruids, struct node_entry host) {
//    string buf;
//    string ASK_INDEX_HEADER = "RI";
//    for (const auto &it : ruids) {
//        buf.assign(it);
//        buf += "\n";
//        buf += ASK_INDEX_HEADER;
//        buf += "\n";
//        buf += host.addr;
//        buf += "\n";
//        buf += to_string(host.port);
//        buf += "\n\n";
//        try {
//            send_queue_async.push(buf);
//        } catch (const char *err) {
//            fprintf(stderr, "%s", err);
//            return false;
//        }
//    }
//
//    return true;
//}