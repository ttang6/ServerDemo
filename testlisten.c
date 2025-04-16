#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;

static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, handle_term);

    if(argc < 3)
    {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));

        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // create a ipv4 socket address
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    ret = listen(sock, backlog);
    printf("Listen to %s port %d\n", ip, port);
    assert(ret != -1);

    // wait for connection, untill SIGTERM stop it
    while(!stop)
    {
        sleep(1);
    }

    // close socket
    close(sock);

    return 0;
}