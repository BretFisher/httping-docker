/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libintl.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gen.h"
#include "res.h"
#include "error.h"

int resolve_host(const char *host, struct addrinfo **ai, char use_ipv6, int portnr)
{
	int rc = -1;
	char servname[10];
	struct addrinfo myaddr;

	memset(&myaddr, 0, sizeof myaddr);

	/* myaddr.ai_flags = AI_PASSIVE; */
	myaddr.ai_socktype = SOCK_STREAM;
	myaddr.ai_protocol = IPPROTO_TCP;
	myaddr.ai_family = use_ipv6 ? AF_INET6 : AF_INET;
	snprintf(servname, sizeof servname, "%d", portnr);

	rc = getaddrinfo(host, servname, &myaddr, ai);

	if (rc != 0)
		set_error(gettext("Resolving %s %sfailed: %s"), host, use_ipv6 ? gettext("(IPv6) ") : "", gai_strerror(rc));

	return rc;
}

struct addrinfo * select_resolved_host(struct addrinfo *ai, char use_ipv6)
{
	struct addrinfo *p = ai;
	while(p)
	{
		if (p -> ai_family == AF_INET6 && use_ipv6)
			return p;

		if (p -> ai_family == AF_INET)
			return p;

		p = p -> ai_next;
	}

	return NULL;
}

void get_addr(struct addrinfo *ai_use, struct sockaddr_in6 *addr)
{
	memcpy(addr, ai_use->ai_addr, ai_use->ai_addrlen);
}

#define incopy(a)       *((struct in_addr *)a)

int resolve_host_ipv4(const char *host, struct sockaddr_in *addr)
{
	struct hostent *hostdnsentries = gethostbyname(host);

	if (hostdnsentries == NULL)
	{
		set_error(gettext("Problem resolving %s (IPv4): %s"), host, hstrerror(h_errno));

		return -1;
	}

	/* create address structure */
	addr -> sin_family = hostdnsentries -> h_addrtype;
	addr -> sin_addr = incopy(hostdnsentries -> h_addr_list[0]);

	return 0;
}
