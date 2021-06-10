/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <errno.h>
#include <libintl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gen.h"
#include "error.h"

ssize_t read_to(int fd, char *whereto, size_t len, double timeout)
{
	for(;;)
	{
		ssize_t rc;
		struct timeval to;
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		to.tv_sec  = (long)(timeout / 1000.0);
		to.tv_usec = (long)(timeout * 1000.0) % 1000000;

		rc = select(fd + 1, &rfds, NULL, NULL, &to);
		if (rc == 0)
			return RC_TIMEOUT;
		else if (rc == -1)
		{
			if (errno == EAGAIN)
				continue;
			if (errno == EINTR)
				return RC_CTRLC;

			set_error(gettext("read_to::select failed: %s"), strerror(errno));

			return RC_SHORTREAD;
		}

		return read(fd, whereto, len);
	}
}

ssize_t myread(int fd, char *whereto, size_t len, double timeout)
{
	ssize_t cnt=0;

	while(len>0)
	{
		ssize_t rc;
		struct timeval to;
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		to.tv_sec  = (long)(timeout / 1000.0);
		to.tv_usec = (long)(timeout * 1000.0) % 1000000;

		rc = select(fd + 1, &rfds, NULL, NULL, &to);
		if (rc == 0)
			return RC_TIMEOUT;
		else if (rc == -1)
		{
			if (errno == EAGAIN)
				continue;
			if (errno == EINTR)
				return RC_CTRLC;

			set_error(gettext("myread::select failed: %s"), strerror(errno));

			return RC_SHORTREAD;
		}

		if (FD_ISSET(fd, &rfds))
		{
			rc = read(fd, whereto, len);

			if (rc == -1)
			{
				if (errno == EAGAIN)
					continue;
				if (errno == EINTR)
					return RC_CTRLC;

				set_error(gettext("myread::read failed: %s"), strerror(errno));

				return RC_SHORTREAD;
			}
			else if (rc == 0)
				break;
			else
			{
				whereto += rc;
				len -= rc;
				cnt += rc;
			}
		}
	}

	return cnt;
}

ssize_t mywrite(int fd, char *wherefrom, size_t len, double timeout)
{
	ssize_t cnt=0;

	while(len>0)
	{
		ssize_t rc;
		struct timeval to;
		fd_set wfds;

		FD_ZERO(&wfds);
		FD_SET(fd, &wfds);

		to.tv_sec  = (long)(timeout / 1000.0);
		to.tv_usec = (long)(timeout * 1000.0) % 1000000;

		rc = select(fd + 1, NULL, &wfds, NULL, &to);
		if (rc == 0)
			return RC_TIMEOUT;
		else if (rc == -1)
		{
			if (errno == EAGAIN)
				continue;
			if (errno == EINTR)
				return RC_CTRLC;

			set_error(gettext("mywrite::select failed: %s"), strerror(errno));

			return RC_SHORTWRITE;
		}

		rc = write(fd, wherefrom, len);

		if (rc == -1)
		{
			if (errno == EAGAIN)
				continue;
			if (errno == EINTR)
				return RC_CTRLC;

			set_error(gettext("mywrite::write failed: %s"), strerror(errno));

			return RC_SHORTWRITE;
		}
		else if (rc == 0)
			break;
		else
		{
			wherefrom += rc;
			len -= rc;
			cnt += rc;
		}
	}

	return cnt;
}

int set_fd_nonblocking(int fd)
{
        /* set fd to non-blocking */
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		fprintf(stderr, gettext("set_fd_nonblocking failed! (%s)\n"), strerror(errno));

                return -1;
	}

        return 0;
}

int set_fd_blocking(int fd)
{
        /* set fd to blocking */
        if (fcntl(fd, F_SETFL, 0) == -1)
	{
		fprintf(stderr, gettext("set_fd_blocking failed! (%s)\n"), strerror(errno));

                return -1;
	}

        return 0;
}
