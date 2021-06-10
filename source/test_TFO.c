#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int main(void) {
        int sfd = 0;
        int qlen = 5;

        if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
                fprintf(stderr, "socket(): %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

#ifdef TCP_FASTOPEN
        if (setsockopt(sfd, SOL_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen)) == -1) {
                fprintf(stderr, "setsockopt(): %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
#else
        fprintf(stderr, "TCP_FASTOPEN: undefined\n");
        return EXIT_FAILURE;
#endif
        return EXIT_SUCCESS;
}
