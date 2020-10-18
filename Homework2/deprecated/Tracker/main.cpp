#include "main.h"
#include "connection.h"
#include <cstring> //memset
#include <cstdlib> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define SERVER "127.0.0.1"
#define BUFLEN 512	//Max length of buffer


int main(int argc, char **args) {
    tracker_start();
    socketDaemon socket_server(TRACKER_PORT_IN, TRACKER_PORT_OUT);
    string res;
    struct sockaddr_in socket_src;

    while(true) {
        try {
            res.clear();
            socket_server.read_from_s_in(res, socket_src);
            if (res.length() > 0) {
                cout << res << endl;
                socket_server.write_to_s_out(res, socket_src);
            }
        } catch (const char * err) {
            break;
        }
    }

    tracker_stop();
    return 0;
}