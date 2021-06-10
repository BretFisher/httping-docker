/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

void shutdown_ssl(void);
int close_ssl_connection(SSL *const ssl_h);
int READ_SSL(SSL *const ssl_h, char *whereto, int len, const double timeout);
int WRITE_SSL(SSL *const ssl_h, const char *whereto, int len, const double timeout);
int connect_ssl(const int fd, SSL_CTX *const client_ctx, SSL **const ssl_h, BIO **const s_bio, const double timeout, double *const ssl_handshake);
SSL_CTX * initialize_ctx(const char ask_compression);
char * get_fingerprint(SSL *const ssl_h);
int connect_ssl_proxy(const int fd, struct addrinfo *const ai, const double timeout, const char *const proxy_user, const char *const proxy_password, const char *const hostname, const int portnr, char *const tfo);
