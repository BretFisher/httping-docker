/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef NO_SSL
#include <openssl/ssl.h>
#include "mssl.h"
#endif

#include "gen.h"
#include "http.h"
#include "io.h"
#include "utils.h"

int get_HTTP_headers(int socket_h, SSL *ssl_h, char **headers, int *overflow, double timeout)
{
	char *term = NULL;
	int len_in=0, len=4096;
	char *buffer = (char *)malloc(len + 1);
	int rc = RC_OK;

	*headers = NULL;

	memset(buffer, 0x00, len);

	for(;;)
	{
		int rrc = -1;
		int now_n = len - len_in;

#ifndef NO_SSL
		if (ssl_h)
			rrc = SSL_read(ssl_h, &buffer[len_in], now_n);
		else
#endif
			rrc = read_to(socket_h, &buffer[len_in], now_n, timeout);

		if (rrc == 0 || rrc == RC_SHORTREAD)	/* socket closed before request was read? */
		{
			rc = RC_SHORTREAD;
			break;
		}
		else if (rrc < 0)
		{
			free(buffer);
			return rrc;
		}

		len_in += rrc;

		assert(len_in >= 0);
		assert(len_in <= len);

		buffer[len_in] = 0x00;

		if (strstr(buffer, "\r\n\r\n") != NULL)
			break;

		if (len_in >= len)
		{
			len <<= 1;
			buffer = (char *)realloc(buffer, len + 1);
		}
	}

	*headers = buffer;

	term = strstr(buffer, "\r\n\r\n");
	if (term)
		*overflow = len_in - (term - buffer + 4);
	else
		*overflow = 0;

	return rc;
}

int dumb_get_HTTP_headers(int socket_h, char **headers, double timeout)
{
	int len_in=0, len=4096;
	char *buffer = (char *)malloc(len);
	int rc = RC_OK;

	*headers = NULL;

	for(;;)
	{
		int rrc = read_to(socket_h, &buffer[len_in], 1, timeout);
		if (rrc == 0 || rrc == RC_SHORTREAD)	/* socket closed before request was read? */
		{
			rc = RC_SHORTREAD;
			break;
		}
		else if (rrc == RC_TIMEOUT)		/* timeout */
		{
			free(buffer);
			return RC_TIMEOUT;
		}

		len_in += rrc;

		buffer[len_in] = 0x00;
		if (memcmp(&buffer[len_in - 4], "\r\n\r\n", 4) == 0)
			break;
		if (memcmp(&buffer[len_in - 2], "\n\n", 2) == 0) /* broken proxies */
			break;

		if (len_in == (len - 1))
		{
			len <<= 1;
			buffer = (char *)realloc(buffer, len);
		}
	}

	*headers = buffer;

	return rc;
}
