#include <libintl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "error.h"
#include "gen.h"
#include "io.h"
#include "res.h"
#include "tcp.h"

int socks5connect(int fd, struct addrinfo *ai, double timeout, const char *socks5_username, const char *socks5_password, const char *host, int port, char abort_on_resolve_failure)
{
	struct sockaddr_in sai;
	uint32_t addr = 0;
	unsigned char io_buffer[256] = { 0 };
	int io_len = 0, rc = -1;

	if ((rc = connect_to(fd, ai, timeout, NULL, NULL, 0, NULL)) == -1)
		return rc;

	/* inform socks server about the auth. methods we support */
	if (socks5_username != NULL)
	{
		io_buffer[0] = 0x05;	/* version */
		io_buffer[1] = 2;	/* 2 authentication methods */
		io_buffer[2] = 0x00;	/* method 1: no authentication */
		io_buffer[3] = 0x02;	/* method 2: username/password */
		io_len = 4;
	}
	else
	{
		io_buffer[0] = 0x05;	/* version */
		io_buffer[1] = 1;	/* 2 authentication methods */
		io_buffer[2] = 0x00;	/* method 1: no authentication */
		io_len = 3;
	}

	if ((rc = mywrite(fd, (char *)io_buffer, io_len, timeout)) < 0)
		return rc;

	/* wait for reply telling selected authentication method */
	if ((rc = myread(fd, (char *)io_buffer, 2, timeout)) < 0)
		return rc;

	if (io_buffer[0] != 0x05)
	{
		set_error(gettext("socks5connect: reply with requested authentication method does not say version 5 (%02x)"), io_buffer[0]);
		return RC_INVAL;
	}

	if (io_buffer[1] == 0x00)
	{
		/* printf("socks5connect: \"no authentication at all\" selected by server\n"); */
	}
	else if (io_buffer[1] == 0x02)
	{
		/* printf("socks5connect: selected username/password authentication\n"); */
	}
	else
	{
		set_error(gettext("socks5connect: socks5 refuses our authentication methods: %02x"), io_buffer[1]);
		return RC_INVAL;
	}

	/* in case the socks5 server asks us to authenticate, do so */
	if (io_buffer[1] == 0x02)
	{
		int io_len = 0;

		if (socks5_username == NULL || socks5_password == NULL)
		{
			set_error(gettext("socks5connect: socks5 server requests username/password authentication"));
			return RC_INVAL;
		}

		io_buffer[0] = 0x01;	/* version */
		io_len = snprintf((char *)&io_buffer[1], sizeof io_buffer - 1, "%c%s%c%s", (int)strlen(socks5_username), socks5_username, (int)strlen(socks5_password), socks5_password);

		if ((rc = mywrite(fd, (char *)io_buffer, io_len + 1, timeout)) < 0)
		{
			set_error(gettext("socks5connect: failed transmitting username/password to socks5 server"));
			return rc;
		}

		if ((rc = myread(fd, (char *)io_buffer, 2, timeout)) < 0)
		{
			set_error(gettext("socks5connect: failed receiving authentication reply"));
			return rc;
		}

		if (io_buffer[1] != 0x00)
		{
			set_error(gettext("socks5connect: password authentication failed"));
			return RC_INVAL;
		}
	}

	/* ask socks5 server to associate with server */
	io_buffer[0] = 0x05;	/* version */
	io_buffer[1] = 0x01;	/* connect to */
	io_buffer[2] = 0x00;	/* reserved */
	io_buffer[3] = 0x01;	/* ipv4 */

	if (resolve_host_ipv4(host, &sai) == -1)
	{
		if (abort_on_resolve_failure)
			error_exit(gettext("Cannot resolve %s"), host);

		return RC_INVAL;
	}

	addr = ntohl(sai.sin_addr.s_addr);

	io_buffer[4] = (addr >> 24) & 255;
	io_buffer[5] = (addr >> 16) & 255;
	io_buffer[6] = (addr >>  8) & 255;
	io_buffer[7] = (addr      ) & 255;

	io_buffer[8] = (port >> 8) & 255;
	io_buffer[9] = (port     ) & 255;

	if ((rc = mywrite(fd, (char *)io_buffer, 10, timeout)) < 0)
	{
		set_error(gettext("socks5connect: failed to transmit associate request"));
		return rc;
	}

	if ((rc = myread(fd, (char *)io_buffer, 10, timeout)) < 0)
	{
		set_error(gettext("socks5connect: command reply receive failure"));
		return rc;
	}

	/* verify reply */
	if (io_buffer[0] != 0x05)
	{
		set_error(gettext("socks5connect: bind request replies with version other than 0x05 (%02x)"), io_buffer[0]);
		return RC_INVAL;
	}

	if (io_buffer[1] != 0x00)
	{
		set_error(gettext("socks5connect: failed to connect (%02x)"), io_buffer[1]);
		return RC_INVAL;
	}

	if (io_buffer[3] != 0x01)
	{
		set_error(gettext("socks5connect: only accepting bind-replies with IPv4 address (%02x)"), io_buffer[3]);
		return RC_INVAL;
	}

	return RC_OK;
}
