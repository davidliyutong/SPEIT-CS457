/*
	Simple udp client
*/
#include<iostream>    //printf
#include<cstring> //memset
#include<cstdlib> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>

#define SERVER "127.0.0.1"
#define BUFLEN 512    //Max length of buffer
#define TRACKERPORT 9668    //The port on which to send data
#define MY_PORT 7777

int main() {
    struct sockaddr_in si_other;
    int s, i, slen = sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("socket error");
        exit(1);
    }

    struct sockaddr_in si_me;
    si_me.sin_port = htons(MY_PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    si_me.sin_family = AF_INET;
    int status = bind(s, (struct sockaddr *) &si_me, sizeof(si_me));

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(TRACKERPORT);

    if (inet_aton(SERVER, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while (1) {
        printf("Enter message : ");
        fgets(message, BUFLEN, stdin);

        //send the message
        if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == -1) {
            printf("sendto error");
            return 1;
        }

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf, '\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recv(s, buf, BUFLEN, 0) == -1) {
            printf("recvfrom error");
            return 1;
        } else {
            std::cout << "recv: " << buf << std::endl;
        }

        if (fputs(buf, stdout) == EOF) {
            printf("\nStandard output error");
        }
    }

    return 0;
}
