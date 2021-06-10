#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <libintl.h>

#include "gen.h"
#include "main.h"
#include "help.h"
#include "utils.h"

void new_version_alert(void)
{
	char new_version = 0;
	FILE *fh = fopen(SPAM_FILE, "r");
	if (!fh)
		new_version = 1;
	else
	{
		char buffer[4096], *dummy = 0x00;

		fgets(buffer, sizeof buffer, fh);

		fclose(fh);

		dummy = strchr(buffer, '\n');
		if (dummy)
			*dummy = 0x00;

		if (strcmp(buffer, VERSION) != 0)
			new_version = 1;
	}

	if (new_version)
	{
		struct utsname buf;
		FILE *fh = fopen(SPAM_FILE, "w");
		if (fh)
		{
			fprintf(fh, "%s\n", VERSION);

			fclose(fh);
		}

		printf("Welcome to the new HTTPing version " VERSION "!\n\n");
#ifdef NC
		printf("Did you know that with -K you can start a fullscreen GUI version with nice graphs and lots more information? And that you can disable the moving graphs with -D?\n");
#ifndef FW
		printf("And if you compile this program with libfftw3, that it can also show a fourier transform of the measured values?\n");
#endif
#else
		printf("Did you know that if you compile this program with NCURSES, that it then includes a nice GUI with lots more information and graphs?\n");
#endif

#if !defined(TCP_TFO) && defined(linux)
		if (uname(&buf) == 0)
		{
			char **rparts = NULL;
			int n_rparts = 0;

			split_string(buf.release, ".", &rparts, &n_rparts);

			if (n_rparts >= 2 && ((atoi(rparts[0]) >= 3 && atoi(rparts[1]) >= 6) || atoi(rparts[0]) >= 4))
				printf("This program supports TCP Fast Open! (if compiled in and only on Linux kernels 3.6 or more recent) See the readme.txt how to enable this.\n");

			free_splitted_string(rparts, n_rparts);
		}
#endif

		printf("\n\n");
	}
}

void version(void)
{
	fprintf(stderr, gettext("HTTPing v" VERSION ", (C) 2003-2016 folkert@vanheusden.com\n"));
#ifndef NO_SSL
	fprintf(stderr, gettext(" * SSL support included (-l)\n"));
#endif

#ifdef NC
#ifdef FW
	fprintf(stderr, gettext(" * ncurses interface with FFT included (-K)\n"));
#else
	fprintf(stderr, gettext(" * ncurses interface included (-K)\n"));
#endif
#endif

#ifdef TCP_TFO
	fprintf(stderr, gettext(" * TFO (TCP fast open) support included (-F)\n"));
#endif
	fprintf(stderr, gettext("\n"));
}

void format_help(const char *short_str, const char *long_str, const char *descr)
{
	int par_width = SWITCHES_COLUMN_WIDTH, max_wrap_width = par_width / 2, cur_par_width = 0;
	int descr_width = max_x - (par_width + 1);
	char *line = NULL, *p = (char *)descr;
	char first = 1;

	if (long_str && short_str)
		str_add(&line, "%-4s / %s", short_str, long_str);
	else if (long_str)
		str_add(&line, "%s", long_str);
	else
		str_add(&line, "%s", short_str);

	cur_par_width = fprintf(stderr, "%-*s ", par_width, line);

	free(line);

	if (par_width + 1 >= max_x || cur_par_width >= max_x)
	{
		fprintf(stderr, "%s\n", descr);
		return;
	}

	for(;strlen(p);)
	{
		char *n =  NULL, *kn = NULL, *copy = NULL;
		int n_len = 0, len_after_ww = 0, len_before_ww = 0;
		int str_len = 0, cur_descr_width = first ? max_x - cur_par_width : descr_width;

		while(*p == ' ')
			p++;

		str_len = strlen(p);
		if (!str_len)
			break;

		len_before_ww = min(str_len, cur_descr_width);

		n = &p[len_before_ww];
		kn = n;

		if (str_len > cur_descr_width)
		{ 
			while (*n != ' ' && n_len < max_wrap_width)
			{
				n--;
				n_len++;
			}

			if (n_len >= max_wrap_width)
				n = kn;
		}

		len_after_ww = (int)(n - p);
		if (len_after_ww <= 0)
			break;

		copy = (char *)malloc(len_after_ww + 1);
		memcpy(copy, p, len_after_ww);
		copy[len_after_ww] = 0x00;

		if (first)
			first = 0;
		else
			fprintf(stderr, "%*s ", par_width, "");

		fprintf(stderr, "%s\n", copy);

		free(copy);

		p = n;
	}
}

void usage(const char *me)
{
	char *dummy = NULL, has_color = 0;
	char host[256] = { 0 };

	/* where to connect to */
	fprintf(stderr, gettext(" *** where to connect to ***\n"));
	format_help("-g x", "--url", gettext("URL to ping (e.g. -g http://localhost/)"));
	format_help("-h x", "--hostname", gettext("hostname to ping (e.g. localhost) - use either -g or -h"));
	format_help("-p x", "--port", gettext("portnumber (e.g. 80) - use with -h"));
	format_help("-6", "--ipv6", gettext("use IPv6 when resolving/connecting"));
#ifndef NO_SSL
	format_help("-l", "--use-ssl", gettext("connect using SSL. pinging an https URL automatically enables this setting"));
#endif
	fprintf(stderr, gettext("\n"));

	/* proxy settings */
	fprintf(stderr, gettext(" *** proxy settings ***\n"));
	format_help("-x x", "--proxy", gettext("x should be \"host:port\" which are the network settings of the http/https proxy server. ipv6 ip-address should be \"[ip:address]:port\""));
	format_help("-E", NULL, gettext("fetch proxy settings from environment variables"));
	format_help(NULL, "--proxy-user x", gettext("username for authentication against proxy"));
	format_help(NULL, "--proxy-password x", gettext("password for authentication against proxy"));
	format_help(NULL, "--proxy-password-file x", gettext("read password for proxy authentication from file x"));
	format_help("-5", NULL, gettext("proxy is a socks5 server"));
	format_help(NULL, "--proxy-buster x", gettext("adds \"&x=[random value]\" to the request URL"));
	fprintf(stderr, gettext("\n"));

	/* timing settings */
	fprintf(stderr, gettext(" *** timing settings ***\n"));
	format_help("-c x", "--count", gettext("how many times to ping"));
	format_help("-i x", "--interval", gettext("delay between each ping"));
	format_help("-t x", "--timeout", gettext("timeout (default: 30s)"));
	format_help(NULL, "--ai / --adaptive-interval", gettext("execute pings at multiples of interval relative to start, automatically enabled in ncurses output mode"));
	format_help("-f", "--flood", gettext("flood connect (no delays)"));
	fprintf(stderr, gettext("\n"));

	/* http settings */
	fprintf(stderr, gettext(" *** HTTP settings ***\n"));
	format_help("-Z", "--no-cache", gettext("ask any proxies on the way not to cache the requests"));
	format_help(NULL, "--divert-connect", gettext("connect to a different host than in the URL given"));
	format_help(NULL, "--keep-cookies", gettext("return the cookies given by the HTTP server in the following request(s)"));
	format_help(NULL, "--no-host-header", gettext("do not add \"Host:\"-line to the request headers"));
	format_help("-Q", "--persistent-connections", gettext("use a persistent connection, i.e. reuse the same TCP connection for multiple HTTP requests. usually possible when 'Connection: Keep-Alive' is sent by server. adds a 'C' to the output if httping had to reconnect"));
	format_help("-I x", "--user-agent", gettext("use 'x' for the UserAgent header"));
	format_help("-R x", "--referer", gettext("use 'x' for the Referer header"));
	format_help(NULL, "--header", gettext("adds an extra request-header"));
	fprintf(stderr, gettext("\n"));

	/* network settings */
	fprintf(stderr, gettext(" *** networking settings ***\n"));
	format_help(NULL, "--max-mtu", gettext("limit the MTU size"));
	format_help(NULL, "--no-tcp-nodelay", gettext("do not disable Naggle"));
	format_help(NULL, "--recv-buffer", gettext("receive buffer size"));
	format_help(NULL, "--tx-buffer", gettext("transmit buffer size"));
	format_help("-r", "--resolve-once", gettext("resolve hostname only once (useful when pinging roundrobin DNS: also takes the first DNS lookup out of the loop so that the first measurement is also correct)"));
	format_help("-W", NULL, gettext("do not abort the program if resolving failed: keep retrying"));
	format_help("-y x", "--bind-to", gettext("bind to an ip-address (and thus interface) with an optional port"));
#ifdef TCP_TFO
	format_help("-F", "--tcp-fast-open", gettext("\"TCP fast open\" (TFO), reduces the latency of TCP connects"));
#endif
#ifdef linux
	format_help(NULL, "--priority", gettext("set priority of packets"));
#endif
	format_help(NULL, "--tos", gettext("set TOS (type of service)"));
	fprintf(stderr, gettext("\n"));

	/* http authentication */
	fprintf(stderr, gettext(" *** HTTP authentication ***\n"));
	format_help("-A", "--basic-auth", gettext("activate (\"basic\") authentication"));
	format_help("-U x", "--username", gettext("username for authentication"));
	format_help("-P x", "--password", gettext("password for authentication"));
	format_help("-T x", NULL, gettext("read the password fom the file 'x' (replacement for -P)"));
	fprintf(stderr, gettext("\n"));

	/* output settings */
	fprintf(stderr, gettext(" *** output settings ***\n"));
	format_help("-s", "--show-statuscodes", gettext("show statuscodes"));
	format_help("-S", "--split-time", gettext("split measured time in its individual components (resolve, connect, send, etc."));
	format_help(NULL, "--threshold-red", gettext("from what ping value to show the value in red (must be bigger than yellow), only in color mode (-Y)"));
	format_help(NULL, "--threshold-yellow", gettext("from what ping value to show the value in yellow"));
	format_help(NULL, "--threshold-show", gettext("from what ping value to show the results"));
	format_help(NULL, "--timestamp / --ts", gettext("put a timestamp before the measured values, use -v to include the date and -vv to show in microseconds"));
	format_help(NULL, "--aggregate x[,y[,z]]", gettext("show an aggregate each x[/y[/z[/etc]]] seconds"));
#ifndef NO_SSL
	format_help("-z", "--show-fingerprint", gettext("show fingerprint (SSL)"));
#endif
	format_help("-v", NULL, gettext("verbose mode"));
	fprintf(stderr, gettext("\n"));

	/* GET settings */
	fprintf(stderr, gettext(" *** \"GET\" (instead of HTTP \"HEAD\") settings ***\n"));
	format_help("-G", "--get-request", gettext("do a GET request instead of HEAD (read the contents of the page as well)"));
	format_help("-b", "--show-transfer-speed", gettext("show transfer speed in KB/s (use with -G)"));
	format_help("-B", "--show-xfer-speed-compressed", gettext("like -b but use compression if available"));
	format_help("-L x", "--data-limit", gettext("limit the amount of data transferred (for -b) to 'x' (in bytes)"));
	format_help("-X", "--show-kb", gettext("show the number of KB transferred (for -b)"));
	fprintf(stderr, gettext("\n"));

	/* output mode settings */
	fprintf(stderr, gettext(" *** output mode settings ***\n"));
	format_help("-q", "--quiet", gettext("quiet, only returncode"));
	format_help("-m", "--parseable-output", gettext("give machine parseable output (see also -o and -e)"));
	format_help("-M", NULL, gettext("json output, cannot be combined with -m"));
	format_help("-o rc,rc,...", "--ok-result-codes", gettext("what http results codes indicate 'ok' comma separated WITHOUT spaces inbetween default is 200, use with -e"));
	format_help("-e x", "--result-string", gettext("string to display when http result code doesn't match"));
	format_help("-n warn,crit", "--nagios-mode-1 / --nagios-mode-2", gettext("Nagios-mode: return 1 when avg. response time >= warn, 2 if >= crit, otherwhise return 0"));
	format_help("-N x", NULL, gettext("Nagios mode 2: return 0 when all fine, 'x' when anything failes"));
	format_help("-C cookie=value", "--cookie", gettext("add a cookie to the request"));
	format_help("-Y", "--colors", gettext("add colors"));
	format_help("-a", "--audible-ping", gettext("audible ping"));
	fprintf(stderr, gettext("\n"));

	/* GUI/ncurses mode */
#if defined(NC)
	fprintf(stderr, gettext(" *** GUI/ncurses mode settings ***\n"));
	format_help("-K", "--ncurses / --gui", gettext("ncurses/GUI mode"));
#if defined(FW)
	format_help(NULL, "--draw-phase", gettext("draw phase (fourier transform) in gui"));
#endif
	format_help(NULL, "--slow-log", gettext("when the duration is x or more, show ping line in the slow log window (the middle window)"));
	format_help(NULL, "--graph-limit x", gettext("do not scale to values above x"));
	format_help("-D", "--no-graph", gettext("do not show graphs (in ncurses/GUI mode)"));
	fprintf(stderr, gettext("\n"));
#endif

	format_help("-V", "--version", gettext("show the version"));
	fprintf(stderr, gettext("\n"));

	dummy = getenv("TERM");
	if (dummy)
	{
		if (strstr(dummy, "ANSI") || strstr(dummy, "xterm") || strstr(dummy, "screen"))
			has_color = 1;
	}

	if (gethostname(host, sizeof host))
		strcpy(host, "localhost");

	fprintf(stderr, gettext("Example:\n"));
	fprintf(stderr, "\t%s %s%s -s -Z\n\n", me, host, has_color ? " -Y" : "");

	new_version_alert();
}
