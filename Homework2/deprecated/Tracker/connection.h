//
// Created by liyutong on 2020/9/24.
//
#include <queue>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <array>
#include "main.h"


#define TRACKER_PORT_IN 9668
#define TRACKER_PORT_OUT 9669
#define BUFFLEN 512
#define MAX_RECV_QUEUE_SIZE 0x10000

using namespace std;

class socketDaemon{
public:
    bool recv_s_in(queue<socketMsg> recv_queue);
    bool send_s_out(queue<socketMsg>  send_queue);
    bool init_socket(int &s, struct sockaddr_in si, int port);
    bool read_from_s_in(string &res, sockaddr_in &si_src);
    bool write_to_s_out(string &msg, sockaddr_in &si_dst);
    bool clear_buf();

    socketDaemon(int port_in, int port_out) {
        socket_in = 0;
        socket_out = 0;

        init_socket(socket_in, socket_info_in, port_in);
        init_socket(socket_out, socket_info_out, port_out);
        clear_buf();
    }
private:
    struct sockaddr_in socket_info_in{}, socket_info_out{}, socket_info_src{}, socket_info_dst{};
    string string_in_tmp;
    string string_out_tmp;
    int socket_in, socket_out;
    char buf[BUFFLEN];


};

bool socketDaemon::init_socket(int &s, struct sockaddr_in si, int port) {
    //create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("socket init failed");
        exit(1);
    }

    ;
    // zero out the structure
    memset((char *) &si, 0, sizeof(si));

    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if (bind(s, (struct sockaddr *) &si, sizeof(si)) == -1) {
        printf("bind failed");
        exit(1);
    }
}




bool socketDaemon::read_from_s_in(string &res, sockaddr_in &si_src) {
    int status;
    int slen = sizeof(si_src);
    clear_buf();
    status = recvfrom(socket_in, buf, BUFFLEN, 0, (struct sockaddr *) &si_src, (socklen_t *) &slen);
    res.assign(buf);
    return true;
}

bool socketDaemon::write_to_s_out(string &msg, struct sockaddr_in &si_dst) {
    int send_len = msg.length();
    int slen = sizeof(si_dst);
    int status = 0;
    status = sendto(socket_out, msg.c_str(), msg.length(), 0, (struct sockaddr *) &si_dst, sizeof(si_dst));
    return true;
}

bool socketDaemon::clear_buf() {
    memset(&buf, BUFFLEN, 0);
    return true;
}

struct sockaddr_in gen_sockaddr(string &address, int port) {
    struct sockaddr_in si{};
    si.sin_family = AF_INET;
    si.sin_port = port;
    if (inet_aton(address.c_str() , &si.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    return si;
}