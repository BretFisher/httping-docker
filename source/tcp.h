/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

int create_socket(struct sockaddr *bind_to, struct addrinfo *ai, int recv_buffer_size, int tx_buffer_size, int max_mtu, char use_no_delay, int priority, int tos);
int connect_to(int fd, struct addrinfo *ai, double timeout, char *tfo, char *msg, int msg_len, char *msg_accepted);
void failure_close(int fd);
