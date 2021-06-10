/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <libintl.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "gen.h"
#include "io.h"
#include "main.h"
#include "tcp.h"

void failure_close(int fd)
{
	struct linger sl;

	sl.l_onoff = 1;
	sl.l_linger = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof sl) == -1)
		set_error(gettext("could not set TCP_NODELAY on socket (%s)"), strerror(errno));

	close(fd);
}

int set_no_delay(int fd)
{
	int flag = 1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) == -1)
	{
		set_error(gettext("could not set TCP_NODELAY on socket (%s)"), strerror(errno));
		return -1;
	}

	return 0;
}

int create_socket(struct sockaddr *bind_to, struct addrinfo *ai, int recv_buffer_size, int tx_buffer_size, int max_mtu, char use_no_delay, int priority, int tos)
{
	int fd = -1;

	/* create socket */
	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (fd == -1)
	{
		set_error(gettext("problem creating socket (%s)"), strerror(errno));
		return RC_INVAL;
	}

	/* go through a specific interface? */
	if (bind_to)
	{
		int set = 1;

		/* set reuse flags */
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &set, sizeof set) == -1)
		{
			set_error(gettext("error setting sockopt to interface (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}

		if (bind(fd, bind_to, sizeof *bind_to) == -1)
		{
			set_error(gettext("error binding to interface (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}
	}

	if (max_mtu >= 0)
	{
		if (setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &max_mtu, sizeof max_mtu) == -1)
		{
			set_error(gettext("error setting MTU size (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}
	}

	if (use_no_delay)
	{
		int rc = -1;

		if ((rc = set_no_delay(fd)) != 0)
			return rc;
	}

	if (tx_buffer_size > 0)
        {
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&tx_buffer_size, sizeof tx_buffer_size) == -1)
		{
			set_error(gettext("error setting transmit buffer size (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}
	}

	if (recv_buffer_size > 0)
        {
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&recv_buffer_size, sizeof recv_buffer_size) == -1)
		{
			set_error(gettext("error setting receive buffer size (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}
	}

#ifdef linux
	if (priority >= 0)
	{
		if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (char *)&priority, sizeof priority) == -1)
		{
			set_error(gettext("error setting priority (%s)"), strerror(errno));
			close(fd);
			return RC_INVAL;
		}
	}
#endif

	if (tos >= 0)
	{
		if (setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof tos) == -1)
		{
			set_error(gettext("failed to set TOS info"));
			close(fd);
			return RC_INVAL;
		}
	}

	return fd;
}

int connect_to(int fd, struct addrinfo *ai, double timeout, char *tfo, char *msg, int msg_len, char *msg_accepted)
{
	int rc = -1;
	struct timeval to;
	fd_set wfds;

	/* make fd nonblocking */
	if (set_fd_nonblocking(fd) == -1)
		return RC_INVAL;

	/* wait for connection */
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	to.tv_sec  = (long)(timeout / 1000.0);
	to.tv_usec = (long)(timeout * 1000.0) % 1000000;

	/* connect to peer */
#ifdef TCP_TFO
	if (tfo && *tfo)
	{
		rc = sendto(fd, msg, msg_len, MSG_FASTOPEN, ai -> ai_addr, ai -> ai_addrlen);
		
		if(rc == msg_len)
			*msg_accepted = 1;
		if(errno == 0)
			return RC_OK;
		if(errno == ENOTSUP)
		{
			printf(gettext("TCP TFO Not Supported. Please check if \"/proc/sys/net/ipv4/tcp_fastopen\" is 1. Disabling TFO for now.\n"));
			*tfo = 0;
			goto old_connect;
		}
	}
			
	else
#else
	(void)tfo;
	(void)msg;
	(void)msg_len;
	(void)msg_accepted;
#endif
	{
		int rc = -1;

old_connect:
		rc = connect(fd, ai -> ai_addr, ai -> ai_addrlen);

		if (rc == 0)
		{
			/* connection made, return */
			return RC_OK;
		}

		if (rc == -1)
		{
			/* problem connecting */
			if (errno != EINPROGRESS)
			{
				set_error(gettext("problem connecting to host: %s"), strerror(errno));
				return RC_INVAL;
			}
		}
	}

	if (stop)
		return RC_CTRLC;

	/* wait for connection */
	rc = select(fd + 1, NULL, &wfds, NULL, &to);
	if (rc == 0)
	{
		set_error(gettext("connect time out"));
		return RC_TIMEOUT;	/* timeout */
	}
	else if (rc == -1)
	{
		if (errno == EINTR)
			return RC_CTRLC;/* ^C pressed */

		set_error(gettext("select() failed: %s"), strerror(errno));

		return RC_INVAL;	/* error */
	}
	else
	{
		int optval=0;
		socklen_t optvallen = sizeof optval;

		/* see if the connect succeeded or failed */
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optvallen) == -1)
		{
			set_error(gettext("getsockopt failed (%s)"), strerror(errno));
			return RC_INVAL;
		}

		/* no error? */
		if (optval == 0)
			return RC_OK;

		/* don't ask */
		errno = optval;
	}

	set_error(gettext("could not connect (%s)"), strerror(errno));

	return RC_INVAL;
}
