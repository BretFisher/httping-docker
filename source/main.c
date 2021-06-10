/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#ifndef NO_SSL
#include <openssl/ssl.h>
#include "mssl.h"
#endif
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#if defined(sun) || defined(__sun)
#include <sys/termios.h>
#endif
#ifdef NC
#include <ncurses.h>
#endif

#include "gen.h"
#include "help.h"
#include "colors.h"
#include "http.h"
#include "io.h"
#include "tcp.h"
#include "res.h"
#include "utils.h"
#include "error.h"
#include "socks5.h"
#ifdef NC
#include "nc.h"
#endif
#include "cookies.h"

volatile int stop = 0;

int quiet = 0;
char machine_readable = 0;
char json_output = 0;
char show_ts = 0;

int max_x = 80, max_y = 24;

char nagios_mode = 0;
char ncurses_mode = 0;

int fd = -1;

volatile char got_sigquit = 0;

void handler_quit(int s)
{
	signal(SIGQUIT, handler_quit);

	got_sigquit = 1;
}

void determine_terminal_size(int *max_y, int *max_x)
{
        struct winsize size;

        *max_x = *max_y = 0;

	if (!isatty(1))
	{
		*max_y = 24;
		*max_x = 80;
	}
#ifdef TIOCGWINSZ
        else if (ioctl(1, TIOCGWINSZ, &size) == 0)
        {
                *max_y = size.ws_row;
                *max_x = size.ws_col;
        }
#endif

        if (!*max_x || !*max_y)
        {
                char *dummy = getenv("COLUMNS");
                if (dummy)
                        *max_x = atoi(dummy);
                else
                        *max_x = 80;

                dummy = getenv("LINES");
                if (dummy)
                        *max_y = atoi(dummy);
                else
                        *max_y = 24;
        }
}

void emit_statuslines(double run_time)
{
#ifdef NC
	if (ncurses_mode)
	{
		time_t t = time(NULL);
		char *t_str = ctime(&t);
		char *dummy = strchr(t_str, '\n');

		if (dummy)
			*dummy = 0x00;

		status_line(gettext("%s, run time: %.3fs, press ctrl + c to stop"), t_str, run_time);
	}
#else
	(void)run_time;
#endif
}

void emit_headers(char *in)
{
#ifdef NC
	static char shown = 0;
	int len_in = -1;

	if (!shown && ncurses_mode && in != NULL && (len_in = strlen(in) - 4) > 0)
	{
		int pos = 0, pos_out = 0;
		char *copy = (char *)malloc(len_in + 1), *dummy = NULL;

		for(pos=0; pos<len_in; pos++)
		{
			if (in[pos] != '\r')
				copy[pos_out++] = in[pos];
		}

		copy[pos_out] = 0x00;

		/* in case more than the headers were sent */
		dummy = strstr(copy, "\n\n");
		if (dummy)
			*dummy = 0x00;

		slow_log("\n%s", copy);

		free(copy);

		shown = 1;
	}
#else
	(void)in;
#endif
}

void emit_json(char ok, int seq, double start_ts, stats_t *t_resolve, stats_t *t_connect, stats_t *t_request, int http_code, const char *msg, int header_size, int data_size, double Bps, const char *host, const char *ssl_fp, double toff_diff_ts, char tfo_success, stats_t *t_ssl, stats_t *t_write, stats_t *t_close, int n_cookies, stats_t *stats_to, stats_t *tcp_rtt_stats, int re_tx, int pmtu, int recv_tos, stats_t *t_total)
{
	if (seq > 1)
		printf(", \n");
	printf("{ ");
	printf("\"status\" : \"%d\", ", ok);
	printf("\"seq\" : \"%d\", ", seq);
	printf("\"start_ts\" : \"%f\", ", start_ts);
	if (t_resolve!=NULL && t_resolve -> cur_valid)
		printf("\"resolve_ms\" : \"%e\", ", t_resolve -> cur);
	else
		printf("\"resolve_ms\" : \"%e\", ",-1.0);
	if (t_connect!=NULL && t_connect -> cur_valid)
		printf("\"connect_ms\" : \"%e\", ", t_connect -> cur);
	else
		printf("\"connect_ms\" : \"%e\", ",-1.0);
	printf("\"request_ms\" : \"%e\", ", t_request -> cur);
	printf("\"total_ms\" : \"%e\", ", t_total -> cur);
	printf("\"http_code\" : \"%d\", ", http_code);
	printf("\"msg\" : \"%s\", ", msg);
	printf("\"header_size\" : \"%d\", ", header_size);
	printf("\"data_size\" : \"%d\", ", data_size);
	printf("\"bps\" : \"%f\", ", Bps);
	printf("\"host\" : \"%s\", ", host);
	printf("\"ssl_fingerprint\" : \"%s\", ", ssl_fp ? ssl_fp : "");
	printf("\"time_offset\" : \"%f\", ", toff_diff_ts);
	printf("\"tfo_success\" : \"%s\", ", tfo_success ? "true" : "false");
	if (t_ssl -> cur_valid)
		printf("\"ssl_ms\" : \"%e\", ", t_ssl -> cur);
	printf("\"tfo_succes\" : \"%s\", ", tfo_success ? "true" : "false");
	if (t_ssl !=NULL && t_ssl -> cur_valid)
		printf("\"ssl_ms\" : \"%e\", ", t_ssl -> cur);
	printf("\"write\" : \"%e\", ", t_write -> cur);
	printf("\"close\" : \"%e\", ", t_close -> cur);
	printf("\"cookies\" : \"%d\", ", n_cookies);
	if (stats_to != NULL && stats_to -> cur_valid)
		printf("\"to\" : \"%e\", ", stats_to -> cur);
	if (tcp_rtt_stats !=NULL && tcp_rtt_stats -> cur_valid)
		printf("\"tcp_rtt_stats\" : \"%e\", ", tcp_rtt_stats -> cur);
	printf("\"re_tx\" : \"%d\", ", re_tx);
	printf("\"pmtu\" : \"%d\", ", pmtu);
	printf("\"tos\" : \"%02x\" ", recv_tos);
	printf("}");
}

char *get_ts_str(int verbose)
{
	char buffer[4096] = { 0 };
	struct tm *tvm = NULL;
	struct timeval tv;

	(void)gettimeofday(&tv, NULL);

	tvm = localtime(&tv.tv_sec);

	if (verbose == 1)
		sprintf(buffer, "%04d/%02d/%02d ", tvm -> tm_year + 1900, tvm -> tm_mon + 1, tvm -> tm_mday);
	else if (verbose >= 2)
		sprintf(buffer, "%.6f", get_ts());

	if (verbose <= 1)
		sprintf(&buffer[strlen(buffer)], "%02d:%02d:%02d.%03d", tvm -> tm_hour, tvm -> tm_min, tvm -> tm_sec, (int)(tv.tv_usec / 1000));

	return strdup(buffer);
}

void emit_error(int verbose, int seq, double start_ts)
{
	char *ts = show_ts ? get_ts_str(verbose) : NULL;

#ifdef NC
	if (ncurses_mode)
	{
		slow_log("\n%s%s", ts ? ts : "", get_error());
		update_terminal();
	}
	else
#endif
		if (!quiet && !machine_readable && !nagios_mode && !json_output)
			printf("%s%s%s%s\n", ts ? ts : "", c_error, get_error(), c_normal);

	if (json_output)
		emit_json(0, seq, start_ts, NULL, NULL, NULL, -1, get_error(), -1, -1, -1, "", "", -1, 0, NULL, NULL, NULL, 0, NULL, NULL, 0, 0, 0, NULL);

	clear_error();

	free(ts);

	fflush(NULL);
}

void handler(int sig)
{
#ifdef NC
	if (sig == SIGWINCH)
		win_resize = 1;
	else
#endif
	{
		if (!json_output)
			fprintf(stderr, gettext("Got signal %d\n"), sig);

		stop = 1;
	}
}

char * read_file(const char *file)
{
	char buffer[4096] = { 0 }, *lf = NULL;
	FILE *fh = fopen(file, "rb");
	if (!fh)
		error_exit(gettext("Cannot open password-file %s"), file);

	if (!fgets(buffer, sizeof buffer, fh))
		error_exit(gettext("Problem reading password from file %s"), file);

	fclose(fh);

	lf = strchr(buffer, '\n');
	if (lf)
		*lf = 0x00;

	return strdup(buffer);
}

char * create_http_request_header(const char *get, char use_proxy_host, char get_instead_of_head, char persistent_connections, const char *hostname, const char *useragent, const char *referer, char ask_compression, char no_cache, const char *auth_usr, const char *auth_password, char **static_cookies, int n_static_cookies, char **dynamic_cookies, int n_dynamic_cookies, const char *proxy_buster, const char *proxy_user, const char *proxy_password, char **additional_headers, int n_additional_headers)
{
	int index;
	char *request = NULL;
	char pb[128] = { 0 };

	if (proxy_buster)
	{
		if (strchr(get, '?'))
			pb[0] = '&';
		else
			pb[0] = '?';

		snprintf(pb + 1, sizeof pb - 1, "%s=%ld", proxy_buster, lrand48());
	}

	if (use_proxy_host)
		str_add(&request, "%s %s%s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", get, pb, persistent_connections?'1':'0');
	else
	{
		const char *dummy = get, *slash = NULL;
		if (strncasecmp(dummy, "http://", 7) == 0)
			dummy += 7;
		else if (strncasecmp(dummy, "https://", 7) == 0)
			dummy += 8;

		slash = strchr(dummy, '/');
		if (slash)
			str_add(&request, "%s %s HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", slash, persistent_connections?'1':'0');
		else
			str_add(&request, "%s / HTTP/1.%c\r\n", get_instead_of_head?"GET":"HEAD", persistent_connections?'1':'0');
	}

	if (hostname)
		str_add(&request, "Host: %s\r\n", hostname);

	if (useragent)
		str_add(&request, "User-Agent: %s\r\n", useragent);
	else
		str_add(&request, "User-Agent: HTTPing v" VERSION "\r\n");

	if (referer)
		str_add(&request, "Referer: %s\r\n", referer);

	if (ask_compression)
		str_add(&request, "Accept-Encoding: gzip,deflate\r\n");

	if (no_cache)
	{
		str_add(&request, "Pragma: no-cache\r\n");
		str_add(&request, "Cache-Control: no-cache\r\n");
	}

	/* Basic Authentification */
	if (auth_usr)
	{ 
		char auth_string[256] = { 0 };
		char b64_auth_string[512] = { 0 };

		sprintf(auth_string, "%s:%s", auth_usr, auth_password); 
		enc_b64(auth_string, strlen(auth_string), b64_auth_string);

		str_add(&request, "Authorization: Basic %s\r\n", b64_auth_string);
	}

	/* proxy authentication */
	if (proxy_user)
	{ 
		char ppa_string[256] = { 0 };
		char b64_ppa_string[512] = { 0 };

		sprintf(ppa_string, "%s:%s", proxy_user, proxy_password); 
		enc_b64(ppa_string, strlen(ppa_string), b64_ppa_string);

		str_add(&request, "Proxy-Authorization: Basic %s\r\n", b64_ppa_string);
	}

	/* Cookie insertion */
	for(index=0; index<n_static_cookies; index++)
		str_add(&request, "Cookie: %s\r\n", static_cookies[index]);
	for(index=0; index<n_dynamic_cookies; index++)
		str_add(&request, "Cookie: %s\r\n", dynamic_cookies[index]);

	for(index=0; index<n_additional_headers; index++)
		str_add(&request, "%s\r\n", additional_headers[index]);

	if (persistent_connections)
		str_add(&request, "Connection: keep-alive\r\n");

	str_add(&request, "\r\n");

	return request;
}

void interpret_url(const char *in, char **path, char **hostname, int *portnr, char use_ipv6, char use_ssl, char **complete_url, char **auth_user, char **auth_password)
{
	char in_use[65536] = { 0 }, *dummy = NULL;

	if (strlen(in) >= sizeof in_use)
		error_exit(gettext("URL too big, HTTPing has a %d bytes limit"), sizeof in_use - 1);

	/* make url complete, if not already */
	if (strncasecmp(in, "http://", 7) == 0 || strncasecmp(in, "https://", 8) == 0) /* complete url? */
	{
		snprintf(in_use, sizeof in_use - 1, "%s", in);

		if (strchr(&in[8], '/') == NULL)
			in_use[strlen(in_use)] = '/';
	}
	else if (strchr(in, '/')) /* hostname + location without 'http://'? */
		sprintf(in_use, "http://%s", in);
	else if (use_ssl)
		sprintf(in_use, "https://%s/", in);
	else
		sprintf(in_use, "http://%s/", in);

	/* sanity check */
	if (strncasecmp(in_use, "http://", 7) == 0 && use_ssl)
		error_exit(gettext("using \"http://\" with SSL enabled (-l)"));

	*complete_url = strdup(in_use);

	/* fetch hostname */
	if (strncasecmp(in_use, "http://", 7) == 0)
		*hostname = strdup(&in_use[7]);
	else /* https */
		*hostname = strdup(&in_use[8]);

	dummy = strchr(*hostname, '/');
	if (dummy)
		*dummy = 0x00;

	/* fetch port number */
	if (use_ssl || strncasecmp(in, "https://", 8) == 0)
		*portnr = 443;
	else
		*portnr = 80;

	if (!use_ipv6)
	{
		char *at = strchr(*hostname, '@');
		char *colon = strchr(*hostname, ':');
		char *colon2 = colon ? strchr(colon + 1, ':') : NULL;

		if (colon2)
		{
			*colon2 = 0x00;
			*portnr = atoi(colon2 + 1);

			if (at)
			{
				*colon = 0x00;
				*at = 0x00;

				*auth_user = strdup(*hostname);
				*auth_password = strdup(colon + 1);
			}
		}
		else if (colon)
		{
			if (colon < at)
			{
				*colon = 0x00;
				*at = 0x00;

				*auth_user = strdup(*hostname);
				*auth_password = strdup(colon + 1);
			}
			else if (at)
			{
				*at = 0x00;
				*auth_user = strdup(*hostname);
			}
			else
			{
				*colon = 0x00;
				*portnr = atoi(colon + 1);
			}
		}
	}

	/* fetch path */
	dummy = strchr(&in_use[8], '/');
	if (dummy)
		*path = strdup(dummy);
	else
		*path = strdup("/");
}

typedef struct {
	int interval, last_ts;
	double value, sd, min, max;
	int n_values;
} aggregate_t;

void set_aggregate(char *in, int *n_aggregates, aggregate_t **aggregates)
{
	char *dummy = in;

	*n_aggregates = 0;

	for(;dummy;)
	{
		(*n_aggregates)++;

		*aggregates = (aggregate_t *)realloc(*aggregates, *n_aggregates * sizeof(aggregate_t));

		memset(&(*aggregates)[*n_aggregates - 1], 0x00, sizeof(aggregate_t));

		(*aggregates)[*n_aggregates - 1].interval = atoi(dummy);
		(*aggregates)[*n_aggregates - 1].max = -MY_DOUBLE_INF;
		(*aggregates)[*n_aggregates - 1].min =  MY_DOUBLE_INF;

		dummy = strchr(dummy, ',');
		if (dummy)
			dummy++;
	}
}

void do_aggregates(double cur_ms, int cur_ts, int n_aggregates, aggregate_t *aggregates, int verbose, char show_ts)
{
	int index=0;

	/* update measurements */
	for(index=0; index<n_aggregates; index++)
	{
		aggregates[index].value += cur_ms;

		if (cur_ms < aggregates[index].min)
			aggregates[index].min = cur_ms;

		if (cur_ms > aggregates[index].max)
			aggregates[index].max = cur_ms;

		aggregates[index].sd += cur_ms * cur_ms;

		aggregates[index].n_values++;
	}

	/* emit */
	for(index=0; index<n_aggregates && cur_ts > 0; index++)
	{
		aggregate_t *a = &aggregates[index];

		if (cur_ts - a -> last_ts >= a -> interval)
		{
			char *line = NULL;
			double avg = a -> n_values ? a -> value / (double)a -> n_values : -1.0;
			char *ts = get_ts_str(verbose);

			str_add(&line, "%s", show_ts ? ts : "");
			free(ts);

			str_add(&line, gettext("AGG[%d]: %d values, min/avg/max%s = %.1f/%.1f/%.1f"), a -> interval, a -> n_values, verbose ? gettext("/sd") : "", a -> min, avg, a -> max);

			if (verbose)
			{
				double sd = -1.0;

				if (a -> n_values)
					sd = sqrt((a -> sd / (double)a -> n_values) - pow(avg, 2.0));

				str_add(&line, "/%.1f", sd);
			}

			str_add(&line, " ms");

#ifdef NC
			if (ncurses_mode)
				slow_log("\n%s", line);
			else
#endif
				printf("%s\n", line);

			free(line);

			aggregates[index].value =
				aggregates[index].sd    = 0.0;
			aggregates[index].min =  MY_DOUBLE_INF;
			aggregates[index].max = -MY_DOUBLE_INF;
			aggregates[index].n_values = 0;
			aggregates[index].last_ts = cur_ts;
		}
	}
}

void fetch_proxy_settings(char **proxy_user, char **proxy_password, char **proxy_host, int *proxy_port, char use_ssl, char use_ipv6)
{
	char *str = getenv(use_ssl ? "https_proxy" : "http_proxy");

	if (!str)
	{
		/* FIXME from wgetrc/curlrc? */
	}

	if (str)
	{
		char *path = NULL, *url = NULL;

		interpret_url(str, &path, proxy_host, proxy_port, use_ipv6, use_ssl, &url, proxy_user, proxy_password);

		free(url);
		free(path);
	}
}

void parse_nagios_settings(const char *in, double *nagios_warn, double *nagios_crit)
{
	char *dummy = strchr(in, ',');
	if (!dummy)
		error_exit(gettext("-n: missing parameter\n"));

	*nagios_warn = atof(in);

	*nagios_crit = atof(dummy + 1);
}

void parse_bind_to(const char *in, struct sockaddr_in *bind_to_4, struct sockaddr_in6 *bind_to_6, struct sockaddr_in **bind_to)
{
	char *dummy = strchr(in, ':');

	if (dummy)
	{
		*bind_to = (struct sockaddr_in *)bind_to_6;
		memset(bind_to_6, 0x00, sizeof *bind_to_6);
		bind_to_6 -> sin6_family = AF_INET6;

		if (inet_pton(AF_INET6, in, &bind_to_6 -> sin6_addr) != 1)
			error_exit(gettext("cannot convert ip address '%s' (for -y)\n"), in);
	}
	else
	{
		*bind_to = (struct sockaddr_in *)bind_to_4;
		memset(bind_to_4, 0x00, sizeof *bind_to_4);
		bind_to_4 -> sin_family = AF_INET;

		if (inet_pton(AF_INET, in, &bind_to_4 -> sin_addr) != 1)
			error_exit(gettext("cannot convert ip address '%s' (for -y)\n"), in);
	}
}

time_t parse_date_from_response_headers(const char *in)
{
	char *date = NULL, *komma = NULL;
	if (in == NULL)
		return -1;

	date = strstr(in, "\nDate:");
	komma = date ? strchr(date, ',') : NULL;
	if (date && komma)
	{
		struct tm tm;
		memset(&tm, 0x00, sizeof tm);

		/* 22 Feb 2015 09:13:56 */
		if (strptime(komma + 1, "%d %b %Y %H:%M:%S %Z", &tm))
			return mktime(&tm);
	}

	return -1;
}

int calc_page_age(const char *in, const time_t their_ts)
{
	int age = -1;

	if (in != NULL && their_ts > 0)
	{
		char *date = strstr(in, "\nLast-Modified:");
		char *komma = date ? strchr(date, ',') : NULL;
		if (date && komma)
		{
			struct tm tm;
			memset(&tm, 0x00, sizeof tm);

			/* 22 Feb 2015 09:13:56 */
			if (strptime(komma + 1, "%d %b %Y %H:%M:%S %Z", &tm))
				age = their_ts - mktime(&tm);
		}
	}

	return age;
}

const char *get_location(const char *host, int port, char use_ssl, char *reply)
{
	if (reply)
	{
		char *copy = strdup(reply);
		char *head = strstr(copy, "\nLocation:");
		char *lf = head ? strchr(head + 1, '\n') : NULL;

		if (head)
		{
			char *buffer = NULL;
			char *dest = head + 11;

			if (lf)
				*lf = 0x00;

			if (memcmp(dest, "http", 4) == 0)
				str_add(&buffer, "%s", dest);
			else
				str_add(&buffer, "http%s://%s:%d%s", use_ssl ? "s" : "", host, port, dest);

			free(copy);

			return buffer;
		}

		free(copy);
	}

	return NULL;
}

char check_compressed(const char *reply)
{
	if (reply != NULL)
	{
		char *encoding = strstr(reply, "\nContent-Encoding:");

		if (encoding)
		{
			char *dummy = strchr(encoding + 1, '\r');
			if (dummy) *dummy = 0x00;

			dummy = strchr(encoding + 1, '\n');
			if (dummy) *dummy = 0x00;

			if (strstr(encoding, "gzip") == 0 || strstr(encoding, "deflate") == 0)
				return 1;
		}
	}

	return 0;
}

int nagios_result(int ok, int nagios_mode, int nagios_exit_code, double avg_httping_time, double nagios_warn, double nagios_crit)
{
	if (nagios_mode == 1)
	{
		if (ok == 0)
		{
			printf(gettext("CRITICAL - connecting failed: %s"), get_error());
			return 2;
		}
		else if (avg_httping_time >= nagios_crit)
		{
			printf(gettext("CRITICAL - average httping-time is %.1f\n"), avg_httping_time);
			return 2;
		}
		else if (avg_httping_time >= nagios_warn)
		{
			printf(gettext("WARNING - average httping-time is %.1f\n"), avg_httping_time);
			return 1;
		}

		printf(gettext("OK - average httping-time is %.1f (%s)|ping=%f\n"), avg_httping_time, get_error(), avg_httping_time);

		return 0;
	}
	else if (nagios_mode == 2)
	{
		const char *err = get_error();

		if (ok && err[0] == 0x00)
		{
			printf(gettext("OK - all fine, avg httping time is %.1f|ping=%f\n"), avg_httping_time, avg_httping_time);
			return 0;
		}

		printf(gettext("%s: - failed: %s"), nagios_exit_code == 1?"WARNING":(nagios_exit_code == 2?"CRITICAL":"ERROR"), err);
		return nagios_exit_code;
	}

	return -1;
}

void proxy_to_host_and_port(char *in, char **proxy_host, int *proxy_port)
{
	if (in[0] == '[')
	{
		char *dummy = NULL;

		*proxy_host = strdup(in + 1);

		dummy = strchr(*proxy_host, ']');
		if (dummy)
		{
			*dummy = 0x00;

			/* +2: ']:' */
			*proxy_port = atoi(dummy + 2);
		}
	}
	else
	{
		char *dummy = strchr(in, ':');

		*proxy_host = in;

		if (dummy)
		{
			*dummy=0x00;
			*proxy_port = atoi(dummy + 1);
		}
	}
}

void stats_close(int *fd, stats_t *t_close, char is_failure)
{
	double t_start = get_ts(), t_end = -1;;

	if (is_failure)
		failure_close(*fd);
	else
		close(*fd);

	*fd = -1;

	t_end = get_ts();

	update_statst(t_close, (t_end - t_start) * 1000.0);
}

void add_header(char ***additional_headers, int *n_additional_headers, const char *in)
{
	*additional_headers = (char **)realloc(*additional_headers, (*n_additional_headers + 1) * sizeof(char **));
	(*additional_headers)[*n_additional_headers] = strdup(in);

	(*n_additional_headers)++;
}

void free_headers(char **additional_headers, int n_additional_headers)
{
	int index = 0;

	for(index=0; index<n_additional_headers; index++)
		free(additional_headers[index]);

	free(additional_headers);
}

typedef struct
{
	double Bps_min, Bps_max;
	long long int Bps_avg;
} bps_t;

void stats_line(const char with_header, const char *const complete_url, const int count, const int curncount, const int err, const int ok, double started_at, const char verbose, const stats_t *const t_total, const double avg_httping_time, bps_t *const bps)
{
	double total_took = get_ts() - started_at;
	int dummy = count;

	if (with_header)
		printf(gettext("--- %s ping statistics ---\n"), complete_url);

	if (curncount == 0 && err > 0)
		fprintf(stderr, gettext("internal error! (curncount)\n"));

	if (count == -1)
		dummy = curncount;

	printf(gettext("%s%d%s connects, %s%d%s ok, %s%3.2f%%%s failed, time %s%s%.0fms%s\n"), c_yellow, curncount, c_normal, c_green, ok, c_normal, c_red, (((double)err) / ((double)dummy)) * 100.0, c_normal, c_blue, c_bright, total_took * 1000.0, c_normal);

	if (ok > 0)
	{
		printf(gettext("round-trip min/avg/max%s = %s%.1f%s/%s%.1f%s/%s%.1f%s"), verbose ? gettext("/sd") : "", c_bright, t_total -> min, c_normal, c_bright, avg_httping_time, c_normal, c_bright, t_total -> max, c_normal);

		if (verbose)
		{
			double sd_final = t_total -> n ? sqrt((t_total -> sd / (double)t_total -> n) - pow(avg_httping_time, 2.0)) : -1.0;
			printf("/%.1f", sd_final);
		}

		printf(" ms\n");

		if (bps)
			printf(gettext("Transfer speed: min/avg/max = %s%f%s/%s%f%s/%s%f%s KB\n"), c_bright, bps -> Bps_min / 1024, c_normal, c_bright, (bps -> Bps_avg / (double)ok) / 1024.0, c_normal, c_bright, bps -> Bps_max / 1024.0, c_normal);
	}
}

int main(int argc, char *argv[])
{
	double started_at = -1;
	char do_fetch_proxy_settings = 0;
	char *hostname = NULL;
	char *proxy_host = NULL, *proxy_user = NULL, *proxy_password = NULL;
	int proxy_port = 8080;
	int portnr = 80;
	char *get = NULL, *request = NULL;
	int req_len = 0;
	int c = 0;
	int count = -1, curncount = 0;
	double wait = 1.0;
	char wait_set = 0;
	int audible = 0;
	int ok = 0, err = 0;
	double timeout = 30.0;
	char show_statuscodes = 0;
	char use_ssl = 0;
	const char *ok_str = "200";
	const char *err_str = "-1";
	const char *useragent = NULL;
	const char *referer = NULL;
	const char *auth_password = NULL;
	const char *auth_usr = NULL;
	char **static_cookies = NULL, **dynamic_cookies = NULL;
	int n_static_cookies = 0, n_dynamic_cookies = 0;
	char resolve_once = 0;
	char have_resolved = 0;
	double nagios_warn=0.0, nagios_crit=0.0;
	int nagios_exit_code = 2;
	double avg_httping_time = -1.0;
	int get_instead_of_head = 0;
	char show_Bps = 0, ask_compression = 0;
	bps_t bps;
	int Bps_limit = -1;
	char show_bytes_xfer = 0, show_fp = 0;
	SSL *ssl_h = NULL;
	BIO *s_bio = NULL;
	struct sockaddr_in *bind_to = NULL;
	struct sockaddr_in bind_to_4;
	struct sockaddr_in6 bind_to_6;
	char split = 0, use_ipv6 = 0;
	char persistent_connections = 0, persistent_did_reconnect = 0;
	char no_cache = 0;
	char use_tfo = 0;
	char abort_on_resolve_failure = 1;
	double offset_yellow = -1, offset_red = -1;
	char colors = 0;
	int verbose = 0;
	double offset_show = -1.0;
	char add_host_header = 1;
	char *proxy_buster = NULL;
	char proxy_is_socks5 = 0;
	char *url = NULL, *complete_url = NULL;
	int n_aggregates = 0;
	aggregate_t *aggregates = NULL;
	char *au_dummy = NULL, *ap_dummy = NULL;
	stats_t t_connect, t_request, t_total, t_resolve, t_write, t_ssl, t_close, stats_to, tcp_rtt_stats, stats_header_size;
	char first_resolve = 1;
	double graph_limit = MY_DOUBLE_INF;
	char nc_graph = 1;
	char adaptive_interval = 0;
	double show_slow_log = MY_DOUBLE_INF;
	char use_tcp_nodelay = 1;
	int max_mtu = -1;
	int write_sleep = 500; /* in us (microseconds), determines resolution of transmit time determination */
	char keep_cookies = 0;
	char abbreviate = 0;
	char *divert_connect = NULL;
	int recv_buffer_size = -1, tx_buffer_size = -1;
	int priority = -1, send_tos = -1;
	char **additional_headers = NULL;
	int n_additional_headers = 0;
#ifndef NO_SSL
	SSL_CTX *client_ctx = NULL;
#endif
	struct sockaddr_in6 addr;
	struct addrinfo *ai = NULL, *ai_use = NULL;
	struct addrinfo *ai_proxy = NULL, *ai_use_proxy = NULL;

	static struct option long_options[] =
	{
		{"aggregate",   1, NULL, 9 },
		{"url",		1, NULL, 'g' },
		{"hostname",	1, NULL, 'h' },
		{"port",	1, NULL, 'p' },
		{"proxy",	1, NULL, 'x' },
		{"count",	1, NULL, 'c' },
		{"persistent-connections",	0, NULL, 'Q' },
		{"interval",	1, NULL, 'i' },
		{"timeout",	1, NULL, 't' },
		{"ipv6",	0, NULL, '6' },
		{"show-statuscodes",	0, NULL, 's' },
		{"split-time",	0, NULL, 'S' },
		{"get-request",	0, NULL, 'G' },
		{"show-transfer-speed",	0, NULL, 'b' },
		{"show-xfer-speed-compressed",	0, NULL, 'B' },
		{"data-limit",	1, NULL, 'L' },
		{"show-kb",	0, NULL, 'X' },
		{"no-cache",	0, NULL, 'Z' },
#ifndef NO_SSL
		{"use-ssl",	0, NULL, 'l' },
		{"show-fingerprint",	0, NULL, 'z' },
#endif
		{"flood",	0, NULL, 'f' },
		{"audible-ping",	0, NULL, 'a' },
		{"parseable-output",	0, NULL, 'm' },
		{"ok-result-codes",	1, NULL, 'o' },
		{"result-string",	1, NULL, 'e' },
		{"user-agent",	1, NULL, 'I' },
		{"referer",	1, NULL, 'S' },
		{"resolve-once",0, NULL, 'r' },
		{"nagios-mode-1",	1, NULL, 'n' },
		{"nagios-mode-2",	1, NULL, 'n' },
		{"bind-to",	1, NULL, 'y' },
		{"quiet",	0, NULL, 'q' },
		{"username",	1, NULL, 'U' },
		{"password",	1, NULL, 'P' },
		{"cookie",	1, NULL, 'C' },
		{"colors",	0, NULL, 'Y' },
		{"offset-yellow",	1, NULL, 1   },
		{"threshold-yellow",	1, NULL, 1   },
		{"offset-red",	1, NULL, 2   },
		{"threshold-red",	1, NULL, 2   },
		{"offset-show",	1, NULL, 3   },
		{"show-offset",	1, NULL, 3   },
		{"threshold-show",	1, NULL, 3   },
		{"show-threshold",	1, NULL, 3   },
		{"timestamp",	0, NULL, 4   },
		{"ts",		0, NULL, 4   },
		{"no-host-header",	0, NULL, 5 },
		{"proxy-buster",	1, NULL, 6 },
		{"proxy-user",	1, NULL, 7 },
		{"proxy-password",	1, NULL, 8 },
		{"proxy-password-file",	1, NULL, 10 },
		{"graph-limit",	1, NULL, 11 },
		{"adaptive-interval",	0, NULL, 12 },
		{"ai",	0, NULL, 12 },
		{"slow-log",	0, NULL, 13 },
		{"draw-phase",	0, NULL, 14 },
		{"no-tcp-nodelay",	0, NULL, 15 },
		{"max-mtu", 1, NULL, 16 },
		{"keep-cookies", 0, NULL, 17 },
		{"abbreviate", 0, NULL, 18 },
		{"divert-connect", 1, NULL, 19 },
		{"recv-buffer", 1, NULL, 20 },
		{"tx-buffer", 1, NULL, 21 },
		{"priority", 1, NULL, 23 },
		{"tos", 1, NULL, 24 },
		{"header", 1, NULL, 25 },
#ifdef NC
		{"ncurses",	0, NULL, 'K' },
		{"gui",	0, NULL, 'K' },
#ifdef FW
		{"no-graph",	0, NULL, 'D' },
#endif
#endif
		{"version",	0, NULL, 'V' },
		{"help",	0, NULL, 22 },
		{NULL,		0, NULL, 0   }
	};

	bps.Bps_min = 1 << 30;
	bps.Bps_max = -bps.Bps_min;
	bps.Bps_avg = 0;

	setlocale(LC_ALL, "");
	bindtextdomain("httping", LOCALEDIR);
	textdomain("httping");

	init_statst(&t_resolve);
	init_statst(&t_connect);
	init_statst(&t_write);
	init_statst(&t_request);
	init_statst(&t_total);
	init_statst(&t_ssl);
	init_statst(&t_close);

	init_statst(&stats_to);
#if defined(linux) || defined(__FreeBSD__)
	init_statst(&tcp_rtt_stats);
#endif
	init_statst(&stats_header_size);

	determine_terminal_size(&max_y, &max_x);

	signal(SIGPIPE, SIG_IGN);

	while((c = getopt_long(argc, argv, "DKEA5MvYWT:ZQ6Sy:XL:bBg:h:p:c:i:Gx:t:o:e:falqsmV?I:R:rn:N:zP:U:C:F", long_options, NULL)) != -1)
	{
		switch(c)
		{
			case 25:
				add_header(&additional_headers, &n_additional_headers, optarg);
				break;

			case 24:
				send_tos = atoi(optarg);
				break;

			case 23:
#ifdef linux
				priority = atoi(optarg);
#else
				error_exit("Setting the network priority is only supported on Linux.\n");
#endif
				break;

			case 21:
				tx_buffer_size = atoi(optarg);
				break;

			case 20:
				recv_buffer_size = atoi(optarg);
				break;

			case 19:
				divert_connect = optarg;
				break;

			case 18:
				abbreviate = 1;
				break;

			case 17:
				keep_cookies = 1;
				break;

			case 16:
				max_mtu = atoi(optarg);
				break;

			case 15:
				use_tcp_nodelay = 0;
				break;

#ifdef NC
			case 14:
				draw_phase = 1;
				break;
#endif

			case 13:
				show_slow_log = atof(optarg);
				break;

			case 12:
				adaptive_interval = 1;
				break;

			case 11:
				graph_limit = atof(optarg);
				break;

#ifdef NC
			case 'K':
				ncurses_mode = 1;
				adaptive_interval = 1;
				if (!wait_set)
					wait = 0.5;
				break;
#ifdef FW
			case 'D':
				nc_graph = 0;
				break;
#endif
#endif

			case 'E':
				do_fetch_proxy_settings = 1;
				break;

			case 'A':
				fprintf(stderr, gettext("\n *** -A is no longer required ***\n\n"));
				break;

			case 'M':
				json_output = 1;
				break;

			case 'v':
				verbose++;
				break;

			case 1:
				offset_yellow = atof(optarg);
				break;

			case 2:
				offset_red = atof(optarg);
				break;

			case 3:
				offset_show = atof(optarg);
				break;

			case 4:
				show_ts = 1;
				break;

			case 5:
				add_host_header = 0;
				break;

			case 6:
				proxy_buster = optarg;
				break;

			case '5':
				proxy_is_socks5 = 1;
				break;

			case 7:
				proxy_user = optarg;
				break;

			case 8:
				proxy_password = optarg;
				break;

			case 9:
				set_aggregate(optarg, &n_aggregates, &aggregates);
				break;

			case 10:
				proxy_password = read_file(optarg);
				break;

			case 'Y':
				colors = 1;
				break;

			case 'W':
				abort_on_resolve_failure = 0;
				break;

			case 'T':
				auth_password = read_file(optarg);
				break;

			case 'Z':
				no_cache = 1;
				break;

			case '6':
				use_ipv6 = 1;
				break;

			case 'S':
				split = 1;
				break;

			case 'Q':
				persistent_connections = 1;
				break;

			case 'y':
				parse_bind_to(optarg, &bind_to_4, &bind_to_6, &bind_to);
				break;

			case 'z':
				show_fp = 1;
				break;

			case 'X':
				show_bytes_xfer = 1;
				break;

			case 'L':
				Bps_limit = atoi(optarg);
				break;

			case 'B':
				show_Bps = 1;
				ask_compression = 1;
				break;

			case 'b':
				show_Bps = 1;
				break;

			case 'e':
				err_str = optarg;
				break;

			case 'o':
				ok_str = optarg;
				break;

			case 'x':
				proxy_to_host_and_port(optarg, &proxy_host, &proxy_port);
				break;

			case 'g':
				url = optarg;
				break;

			case 'r':
				resolve_once = 1;
				break;

			case 'h':
				free(url);
				url = NULL;
				str_add(&url, "http://%s/", optarg);
				break;

			case 'p':
				portnr = atoi(optarg);
				break;

			case 'c':
				count = atoi(optarg);
				break;

			case 'i':
				wait = atof(optarg);
				if (wait < 0.0)
					error_exit(gettext("-i cannot have a value smaller than zero"));
				wait_set = 1;
				break;

			case 't':
				timeout = atof(optarg);
				break;

			case 'I':
				useragent = optarg;
				break;

			case 'R':
				referer = optarg;
				break;

			case 'a':
				audible = 1;
				break;

			case 'f':
				wait = 0;
				wait_set = 1;
				adaptive_interval = 0;
				break;

			case 'G':
				get_instead_of_head = 1;
				break;

#ifndef NO_SSL
			case 'l':
				use_ssl = 1;
				break;
#endif

			case 'm':
				machine_readable = 1;
				break;

			case 'q':
				quiet = 1;
				break;

			case 's':
				show_statuscodes = 1;
				break;

			case 'V':
				version();
				return 0;

			case 'n':
				if (nagios_mode)
					error_exit(gettext("-n and -N are mutual exclusive\n"));
				else
					nagios_mode = 1;

				parse_nagios_settings(optarg, &nagios_warn, &nagios_crit);
				break;

			case 'N':
				if (nagios_mode) error_exit(gettext("-n and -N are mutual exclusive\n"));
				nagios_mode = 2;
				nagios_exit_code = atoi(optarg);
				break;

			case 'P':
				auth_password = optarg;
				break;

			case 'U':
				auth_usr = optarg;
				break;

			case 'C':
				add_cookie(&static_cookies, &n_static_cookies, optarg);
				break;

			case 'F':
#ifdef TCP_TFO
				use_tfo = 1;
#else
				fprintf(stderr, gettext("Warning: TCP TFO is not supported. Disabling.\n"));
#endif
				break;

			case 22:
				version();

				usage(argv[0]);

				return 0;

			case '?':
			default:
				fprintf(stderr, "\n");
				version();

				fprintf(stderr, gettext("\n\nPlease run:\n\t%s --help\nto see a list of options.\n\n"), argv[0]);

				return 1;
		}
	}

	if (do_fetch_proxy_settings)
		fetch_proxy_settings(&proxy_user, &proxy_password, &proxy_host, &proxy_port, use_ssl, use_ipv6);

	if (optind < argc)
		url = argv[optind];

	if (!url)
	{
		fprintf(stderr, gettext("No URL/host to ping given\n\n"));
		return 1;
	}

	if (machine_readable + json_output + ncurses_mode > 1)
		error_exit(gettext("Cannot combine -m, -M and -K"));

	if ((machine_readable || json_output) && n_aggregates > 0)
		error_exit(gettext("Aggregates can only be used in non-machine/json-output mode"));

	clear_error();

	if (!(get_instead_of_head || use_ssl) && show_Bps)
		error_exit(gettext("-b/-B can only be used when also using -G (GET instead of HEAD) or -l (use SSL)\n"));

	if (colors)
		set_colors(ncurses_mode);

	if (!machine_readable && !json_output)
		printf("%s%s", c_normal, c_white);

	interpret_url(url, &get, &hostname, &portnr, use_ipv6, use_ssl, &complete_url, &au_dummy, &ap_dummy);
	if (!auth_usr)
		auth_usr = au_dummy;
	if (!auth_password)
		auth_password = ap_dummy;

#ifdef NC
	if (ncurses_mode)
	{
		if (wait == 0.0)
			wait = 0.001;

		init_ncurses_ui(graph_limit, 1.0 / wait, colors);
	}
#endif

	if (strncmp(complete_url, "https://", 8) == 0 && !use_ssl)
	{
		use_ssl = 1;
#ifdef NC
		if (ncurses_mode)
		{
			slow_log(gettext("\nAuto enabling SSL due to https-URL\n"));
			update_terminal();
		}
		else
#endif
		{
			fprintf(stderr, gettext("Auto enabling SSL due to https-URL\n"));
		}
	}

	if (use_tfo && use_ssl)
		error_exit(gettext("TCP Fast open and SSL not supported together\n"));

	if (verbose)
	{
#ifdef NC
		if (ncurses_mode)
		{
			slow_log(gettext("\nConnecting to host %s, port %d and requesting file %s"), hostname, portnr, get);

			if (proxy_host)
				slow_log(gettext("\nUsing proxyserver: %s:%d"), proxy_host, proxy_port);
		}
		else
#endif
		{
			printf(gettext("Connecting to host %s, port %d and requesting file %s\n\n"), hostname, portnr, get);

			if (proxy_host)
				fprintf(stderr, gettext("Using proxyserver: %s:%d\n"), proxy_host, proxy_port);
		}
	}

#ifndef NO_SSL
	if (use_ssl)
	{
		client_ctx = initialize_ctx(ask_compression);
		if (!client_ctx)
		{
			set_error(gettext("problem creating SSL context"));
			goto error_exit;
		}
	}
#endif

	if (!quiet && !machine_readable && !nagios_mode && !json_output)
	{
#ifdef NC
		if (ncurses_mode)
			slow_log("\nPING %s:%d (%s):", hostname, portnr, get);
		else
#endif
			printf("PING %s%s:%s%d%s (%s):\n", c_green, hostname, c_bright, portnr, c_normal, get);
	}

	if (json_output)
		printf("[\n");

	if (adaptive_interval && wait <= 0.0)
		error_exit(gettext("Interval must be > 0 when using adaptive interval"));

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	signal(SIGQUIT, handler_quit);

	timeout *= 1000.0;	/* change to ms */

	/*
	   if (follow_30x)
	   {
	   get headers

	   const char *get_location(const char *host, int port, char use_ssl, char *reply)

	   set new host/port/path/etc
	   }
	 */

	started_at = get_ts();
	if (proxy_host)
	{
#ifdef NC
		if (ncurses_mode)
		{
			slow_log(gettext("\nResolving hostname %s"), proxy_host);
			update_terminal();
		}
#endif

		if (resolve_host(proxy_host, &ai_proxy, use_ipv6, proxy_port) == -1)
			error_exit(get_error());

		ai_use_proxy = select_resolved_host(ai_proxy, use_ipv6);
		if (!ai_use_proxy)
			error_exit(gettext("No valid IPv4 or IPv6 address found for %s"), proxy_host);
	}
	else if (resolve_once)
	{
		char *res_host = divert_connect ? divert_connect : hostname;
#ifdef NC
		if (ncurses_mode)
		{
			slow_log(gettext("\nResolving hostname %s"), res_host);
			update_terminal();
		}
#endif

		if (resolve_host(res_host, &ai, use_ipv6, portnr) == -1)
		{
			err++;
			emit_error(verbose, -1, started_at);
			have_resolved = 0;
			if (abort_on_resolve_failure)
				error_exit(get_error());
		}

		ai_use = select_resolved_host(ai, use_ipv6);
		if (!ai_use)
		{
			set_error(gettext("No valid IPv4 or IPv6 address found for %s"), res_host);

			if (abort_on_resolve_failure)
				error_exit(get_error());

			/* do not emit the resolve-error here: as 'have_resolved' is set to 0
			   next, the program will try to resolve again anyway
			   this prevents a double error-message while err is increased only
			   once
			 */
			have_resolved = 0;
		}

		if (have_resolved)
			get_addr(ai_use, &addr);
	}

	if (persistent_connections)
		fd = -1;

	while((curncount < count || count == -1) && stop == 0)
	{
		double dstart = -1.0, dend = -1.0, dafter_connect = 0.0, dafter_resolve = 0.0, dafter_write_complete = 0.0;
		char *reply = NULL;
		double Bps = 0;
		char is_compressed = 0;
		long long int bytes_transferred = 0;
		time_t their_ts = 0;
		int age = -1;
		char *sc = NULL, *scdummy = NULL;
		char *fp = NULL;
		int re_tx = 0, pmtu = 0, recv_tos = 0;
		socklen_t recv_tos_len = sizeof recv_tos;

		dstart = get_ts();

		for(;;)
		{
			char did_reconnect = 0;
			int rc = -1;
			int persistent_tries = 0;
			int len = 0, overflow = 0, headers_len = 0;
			char req_sent = 0;
			double dummy_ms = 0.0;
			double their_est_ts = -1.0, toff_diff_ts = -1.0;
			char tfo_success = 0;
			double ssl_handshake = 0.0;
			char cur_have_resolved = 0;
#if defined(linux) || defined(__FreeBSD__)
			struct tcp_info info;
			socklen_t info_len = sizeof(struct tcp_info);
#endif

			curncount++;

persistent_loop:
			if ((!resolve_once || (resolve_once == 1 && have_resolved == 0)) && fd == -1 && proxy_host == NULL)
			{
				char *res_host = divert_connect ? divert_connect : hostname;

				memset(&addr, 0x00, sizeof addr);

#ifdef NC
				if (ncurses_mode && first_resolve)
				{
					slow_log(gettext("\nResolving hostname %s"), res_host);
					update_terminal();
					first_resolve = 0;
				}
#endif

				if (ai)
				{
					freeaddrinfo(ai);

					ai_use = ai = NULL;
				}

				if (resolve_host(res_host, &ai, use_ipv6, portnr) == -1)
				{
					err++;
					emit_error(verbose, curncount, dstart);

					if (abort_on_resolve_failure)
						error_exit(get_error());
					break;
				}

				ai_use = select_resolved_host(ai, use_ipv6);
				if (!ai_use)
				{
					set_error(gettext("No valid IPv4 or IPv6 address found for %s"), res_host);
					emit_error(verbose, curncount, dstart);
					err++;

					if (abort_on_resolve_failure)
						error_exit(get_error());

					break;
				}

				get_addr(ai_use, &addr);

				cur_have_resolved = have_resolved = 1;
			}

			if (cur_have_resolved)
			{
				dafter_resolve = get_ts();
				dummy_ms = (dafter_resolve - dstart) * 1000.0;
				update_statst(&t_resolve, dummy_ms);
			}

			free(request);
			request = create_http_request_header(proxy_host ? complete_url : get, proxy_host ? 1 : 0, get_instead_of_head, persistent_connections, add_host_header ? hostname : NULL, useragent, referer, ask_compression, no_cache, auth_usr, auth_password, static_cookies, n_static_cookies, dynamic_cookies, keep_cookies ? n_dynamic_cookies : 0, proxy_buster, proxy_user, proxy_password, additional_headers, n_additional_headers);
			req_len = strlen(request);

			if (req_len >= 4096)
			{
				char *line = NULL;
				static int notify_cnt = 0;

				notify_cnt++;

				if (notify_cnt == MAX_SHOW_SUPPRESSION + 1)
					str_add(&line, gettext("Will no longer inform about request headers too large."));
				else if (notify_cnt <= MAX_SHOW_SUPPRESSION)
					str_add(&line, gettext("Request headers > 4KB! (%d bytes) This may give failures with some HTTP servers."), req_len);

				if (line)
				{
#ifdef NC
					if (ncurses_mode)
						slow_log("\n%s", line);
					else
#endif
						printf("%s\n", line);
				}

				free(line);
			}

			if ((persistent_connections && fd < 0) || !persistent_connections)
			{
				int rc = -1;
				struct addrinfo *ai_dummy = proxy_host ? ai_use_proxy : ai_use;

				fd = create_socket((struct sockaddr *)bind_to, ai_dummy, recv_buffer_size, tx_buffer_size, max_mtu, use_tcp_nodelay, priority, send_tos);
				if (fd < 0)
					rc = fd; /* FIXME need to fix this, this is ugly */
				else if (proxy_host)
				{
					if (proxy_is_socks5)
						rc = socks5connect(fd, ai_dummy, timeout, proxy_user, proxy_password, hostname, portnr, abort_on_resolve_failure);
#ifndef NO_SSL
					else if (use_ssl)
						rc = connect_ssl_proxy(fd, ai_dummy, timeout, proxy_user, proxy_password, hostname, portnr, &use_tfo);
#endif
					else
						rc = connect_to(fd, ai_dummy, timeout, &use_tfo, request, req_len, &req_sent);
				}
				else
				{

					rc = connect_to(fd, ai_dummy, timeout, &use_tfo, request, req_len, &req_sent);
				}

				if (rc < 0)
				{
					failure_close(fd);
					fd = rc; /* FIXME need to fix this */
				}

				did_reconnect = 1;
			}

			if (fd == RC_CTRLC)	/* ^C pressed */
				break;

			if (fd < 0)
			{
				emit_error(verbose, curncount, dstart);
				fd = -1;
			}

			if (fd >= 0)
			{
				/* set fd blocking */
				if (set_fd_blocking(fd) == -1) /* FIXME redundant? already in connect_to etc? */
				{
					stats_close(&fd, &t_close, 1);
					break;
				}

#ifndef NO_SSL
				if (use_ssl && ssl_h == NULL)
				{
					int rc = connect_ssl(fd, client_ctx, &ssl_h, &s_bio, timeout, &ssl_handshake);
					if (rc == 0)
						update_statst(&t_ssl, ssl_handshake);
					else
					{
						stats_close(&fd, &t_close, 1);
						fd = rc;

						if (persistent_connections && ++persistent_tries < 2)
						{
							persistent_did_reconnect = 1;

							goto persistent_loop;
						}
					}
				}
#endif
			}

			dafter_connect = get_ts();

			if (did_reconnect)
			{
				if (cur_have_resolved)
					dummy_ms = (dafter_connect - dafter_resolve) * 1000.0;
				else
					dummy_ms = (dafter_connect - dstart) * 1000.0;

				update_statst(&t_connect, dummy_ms - ssl_handshake);
			}

			if (fd < 0)
			{
				if (fd == RC_TIMEOUT)
					set_error(gettext("timeout connecting to host"));

				emit_error(verbose, curncount, dstart);
				err++;

				fd = -1;

				break;
			}

#ifndef NO_SSL
			if (use_ssl)
				rc = WRITE_SSL(ssl_h, request, req_len, timeout);
			else
#endif
			{
				if (!req_sent)
					rc = mywrite(fd, request, req_len, timeout);
				else
					rc = req_len;
			}

			/* wait for data transmit(!) to complete,
			   e.g. until the transmitbuffers are empty and the data was
			   sent to the next hop
			 */
#ifndef __CYGWIN__
			for(;;)
			{
				int bytes_left = 0;
				int i_rc = ioctl(fd, TIOCOUTQ, &bytes_left);

				if (i_rc == -1 || bytes_left == 0 || stop)
					break;

				dafter_write_complete = get_ts();
				if ((dafter_write_complete - dafter_connect) * 1000.0 >= timeout)
					break;

				/* this keeps it somewhat from becoming a busy loop
				 * I know of no other way to wait for the kernel to
				 * finish the transmission
				 */
				usleep(write_sleep);
			}
#endif

			dafter_write_complete = get_ts();

			dummy_ms = (dafter_write_complete - dafter_connect) * 1000.0;
			update_statst(&t_write, dummy_ms);

			if (rc != req_len)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						stats_close(&fd, &t_close, 0);
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == -1)
					set_error(gettext("error sending request to host"));
				else if (rc == RC_TIMEOUT)
					set_error(gettext("timeout sending to host"));
				else if (rc == RC_INVAL)
					set_error(gettext("retrieved invalid data from host"));
				else if (rc == RC_CTRLC)
				{/* ^C */}
				else if (rc == 0)
					set_error(gettext("connection prematurely closed by peer"));

				emit_error(verbose, curncount, dstart);

				stats_close(&fd, &t_close, 1);
				err++;

				break;
			}

#if defined(linux) || defined(__FreeBSD__)
#ifdef NC
			if (!use_ssl && ncurses_mode)
			{
				struct timeval tv;
				int t_rc = -1;

				fd_set rfds;
				FD_ZERO(&rfds);
				FD_SET(fd, &rfds);

				tv.tv_sec = (long)(timeout / 1000.0) % 1000000;
				tv.tv_usec = (long)(timeout * 1000.0) % 1000000;

				t_rc = select(fd + 1, &rfds, NULL, NULL, &tv);

#ifdef linux
				if (t_rc == 1 && \
						FD_ISSET(fd, &rfds) && \
						getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0 && \
						info.tcpi_unacked > 0)
				{
					static int in_transit_cnt = 0;

					in_transit_cnt++;
					if (in_transit_cnt == MAX_SHOW_SUPPRESSION + 1)
						slow_log(gettext("\nNo longer emitting message about \"still data in transit\""));
					else if (in_transit_cnt <= MAX_SHOW_SUPPRESSION)
						slow_log(gettext("\nHTTP server started sending data with %d bytes still in transit"), info.tcpi_unacked);
				}
#endif

				if (t_rc == 0)
				{
					stats_close(&fd, &t_close, 1);

					rc = RC_TIMEOUT;
					set_error(gettext("timeout sending to host"));

					emit_error(verbose, curncount, dstart);

					err++;

					break;
				}
			}
#endif

			if (getsockopt(fd, IPPROTO_IP, IP_TOS, &recv_tos, &recv_tos_len) == -1)
			{
				set_error(gettext("failed to obtain TOS info"));
				recv_tos = -1;
			}
#endif

			rc = get_HTTP_headers(fd, ssl_h, &reply, &overflow, timeout);

#ifdef NC
			if (ncurses_mode && !get_instead_of_head && overflow > 0)
			{
				static int more_data_cnt = 0;

				more_data_cnt++;
				if (more_data_cnt == MAX_SHOW_SUPPRESSION + 1)
					slow_log(gettext("\nNo longer emitting message about \"more data than response headers\""));
				else if (more_data_cnt <= MAX_SHOW_SUPPRESSION)
					slow_log(gettext("\nHTTP server sent more data than just the response headers"));
			}
#endif

			emit_headers(reply);

			if (reply)
			{
				free_cookies(dynamic_cookies, n_dynamic_cookies);
				dynamic_cookies = NULL;
				n_dynamic_cookies = 0;

				get_cookies(reply, &dynamic_cookies, &n_dynamic_cookies, &static_cookies, &n_static_cookies);
			}

			if ((show_statuscodes || machine_readable || json_output || ncurses_mode) && reply != NULL)
			{
				/* statuscode is in first line behind
				 * 'HTTP/1.x'
				 */
				char *dummy = strchr(reply, ' ');

				if (dummy)
				{
					sc = strdup(dummy + 1);

					/* lines are normally terminated with a
					 * CR/LF
					 */
					dummy = strchr(sc, '\r');
					if (dummy)
						*dummy = 0x00;
					dummy = strchr(sc, '\n');
					if (dummy)
						*dummy = 0x00;
				}
			}

			their_ts = parse_date_from_response_headers(reply);

			age = calc_page_age(reply, their_ts);

			is_compressed = check_compressed(reply);

			if (persistent_connections && show_bytes_xfer && reply != NULL)
			{
				char *length = strstr(reply, "\nContent-Length:");
				if (!length)
				{
					set_error(gettext("'Content-Length'-header missing!"));
					emit_error(verbose, curncount, dstart);
					stats_close(&fd, &t_close, 1);
					break;
				}

				len = atoi(&length[17]);
			}

			headers_len = 0;
			if (reply)
			{
				headers_len = strlen(reply) + 4 - overflow;
				free(reply);
				reply = NULL;
			}

			update_statst(&stats_header_size, headers_len);

			if (rc < 0)
			{
				if (persistent_connections)
				{
					if (++persistent_tries < 2)
					{
						stats_close(&fd, &t_close, 0);
						persistent_did_reconnect = 1;
						goto persistent_loop;
					}
				}

				if (rc == RC_SHORTREAD)
					set_error(gettext("short read during receiving reply-headers from host"));
				else if (rc == RC_TIMEOUT)
					set_error(gettext("timeout while receiving reply-headers from host"));

				emit_error(verbose, curncount, dstart);

				stats_close(&fd, &t_close, 1);
				err++;

				break;
			}

			ok++;

			if (get_instead_of_head && show_Bps)
			{
				int buffer_size = RECV_BUFFER_SIZE;
				char *buffer = (char *)malloc(buffer_size);
				double dl_start = get_ts(), dl_end;
				double cur_limit = Bps_limit;

				if (persistent_connections)
				{
					if (cur_limit == -1 || len < cur_limit)
						cur_limit = len - overflow;
				}

				for(;;)
				{
					int n = cur_limit != -1 ? min(cur_limit - bytes_transferred, buffer_size) : buffer_size;
					int rc = read(fd, buffer, n);

					if (rc == -1)
					{
						if (errno != EINTR && errno != EAGAIN)
							error_exit(gettext("read of response body dataa failed"));
					}
					else if (rc == 0)
					{
						break;
					}

					bytes_transferred += rc;

					if (cur_limit != -1 && bytes_transferred >= cur_limit)
						break;
				}

				free(buffer);

				dl_end = get_ts();

				Bps = (double)bytes_transferred / max(dl_end - dl_start, 0.000001);
				bps.Bps_min = min(bps.Bps_min, Bps);
				bps.Bps_max = max(bps.Bps_max, Bps);
				bps.Bps_avg += Bps;
			}

			dend = get_ts();

#ifndef NO_SSL
			if (use_ssl)
			{
				if ((show_fp || json_output || ncurses_mode) && ssl_h != NULL)
					fp = get_fingerprint(ssl_h);

				if (!persistent_connections)
				{
					if (close_ssl_connection(ssl_h) == -1)
					{
						set_error(gettext("error shutting down ssl"));
						emit_error(verbose, curncount, dstart);
					}

					SSL_free(ssl_h);
					ssl_h = NULL;
					s_bio = NULL;
				}
			}
#endif

#if defined(linux) || defined(__FreeBSD__)
			if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0)
			{
#ifdef TCP_TFO
				if (info.tcpi_options & TCPI_OPT_SYN_DATA)
					tfo_success = 1;
#endif

				update_statst(&tcp_rtt_stats, (double)info.tcpi_rtt / 1000.0);

#ifdef linux
				re_tx = info.tcpi_retransmits;
				pmtu = info.tcpi_pmtu;
#endif
			}
#endif

			if (!persistent_connections)
				stats_close(&fd, &t_close, 0);

			dummy_ms = (dend - dafter_write_complete) * 1000.0;
			update_statst(&t_request, dummy_ms);

			dummy_ms = (dend - dstart) * 1000.0;
			update_statst(&t_total, dummy_ms);

			their_est_ts = (dend + dafter_connect) / 2.0; /* estimate of when other end started replying */
			toff_diff_ts = ((double)their_ts - their_est_ts) * 1000.0;
			update_statst(&stats_to, toff_diff_ts);

			if (json_output)
			{
				char current_host[4096] = { 0 };

				if (proxy_host)
					snprintf(current_host, sizeof current_host, "%s", hostname);
				else if (getnameinfo((const struct sockaddr *)&addr, sizeof addr, current_host, sizeof current_host, NULL, 0, NI_NUMERICHOST) == -1)
					snprintf(current_host, sizeof current_host, gettext("getnameinfo() failed: %d (%s)"), errno, strerror(errno));

				emit_json(1, curncount, dstart, &t_resolve, &t_connect, &t_request, atoi(sc ? sc : "-1"), sc ? sc : "?", headers_len, len, Bps, current_host, fp, toff_diff_ts, tfo_success, &t_ssl, &t_write, &t_close, n_dynamic_cookies, &stats_to, &tcp_rtt_stats, re_tx, pmtu, recv_tos, &t_total);
			}
			else if (machine_readable)
			{
				if (sc)
				{
					char *dummy = strchr(sc, ' ');
					if (dummy)
						*dummy = 0x00;

					if (strstr(ok_str, sc))
						printf("%f", t_total.cur);
					else
						printf("%s", err_str);

					if (show_statuscodes)
						printf(" %s", sc);
				}
				else
				{
					printf("%s", err_str);
				}

				if(audible)
					putchar('\a');

				printf("\n");
			}
			else if (!quiet && !nagios_mode && t_total.cur >= offset_show)
			{
				char *tot_str = NULL;
				const char *i6_bs = "", *i6_be = "";
				char *line = NULL;
				const char *ms_color = c_green;
				char current_host[4096] = { 0 };
				char *operation = !persistent_connections ? gettext("connected to") : gettext("pinged host");
				const char *sep = c_bright, *unsep = c_normal;

				if (show_ts || ncurses_mode)
				{
					char *ts = get_ts_str(verbose);

					if (ncurses_mode)
						str_add(&line, "[%s] ", ts);
					else
						str_add(&line, "%s ", ts);

					free(ts);
				}

				if (curncount & 1)
				{
					str_add(&line, "%s", c_bright);

					sep = c_normal;
					unsep = c_bright;
				}

				if (proxy_host)
					snprintf(current_host, sizeof current_host, "%s", hostname);
				else if (getnameinfo((const struct sockaddr *)&addr, sizeof addr, current_host, sizeof current_host, NULL, 0, NI_NUMERICHOST) == -1)
					snprintf(current_host, sizeof current_host, gettext("getnameinfo() failed: %d (%s)"), errno, strerror(errno));

				if (addr.sin6_family == AF_INET6)
				{
					i6_bs = "[";
					i6_be = "]";
				}

				if (offset_red > 0.0 && t_total.cur >= offset_red)
					ms_color = c_red;
				else if (offset_yellow > 0.0 && t_total.cur >= offset_yellow)
					ms_color = c_yellow;

				if (!ncurses_mode)
					str_add(&line, "%s%s ", c_white, operation);

				if (persistent_connections && show_bytes_xfer)
					str_add(&line, gettext("%s%s%s%s%s:%s%d%s (%d/%d bytes), seq=%s%d%s "), c_red, i6_bs, current_host, i6_be, c_white, c_yellow, portnr, c_white, headers_len, len, c_blue, curncount-1, c_white);
				else
					str_add(&line, gettext("%s%s%s%s%s:%s%d%s (%d bytes), seq=%s%d%s "), c_red, i6_bs, current_host, i6_be, c_white, c_yellow, portnr, c_white, headers_len, c_blue, curncount-1, c_white);

				tot_str = format_value(t_total.cur, 6, 2, abbreviate);

				if (split)
				{
					char *res_str = t_resolve.cur_valid ? format_value(t_resolve.cur, 6, 2, abbreviate) : strdup(gettext("   n/a"));
					char *con_str = t_connect.cur_valid ? format_value(t_connect.cur, 6, 2, abbreviate) : strdup(gettext("   n/a"));
					char *wri_str = format_value(t_write.cur, 6, 2, abbreviate);
					char *req_str = format_value(t_request.cur, 6, 2, abbreviate);
					char *clo_str = format_value(t_close.cur, 6, 2, abbreviate);

					str_add(&line, gettext("time=%s+%s+%s+%s+%s%s=%s%s%s%s ms %s%s%s"), res_str, con_str, wri_str, req_str, clo_str, sep, unsep, ms_color, tot_str, c_white, c_cyan, sc?sc:"", c_white);

					free(clo_str);
					free(req_str);
					free(wri_str);
					free(con_str);
					free(res_str);
				}
				else
				{
					str_add(&line, gettext("time=%s%s%s ms %s%s%s"), ms_color, tot_str, c_white, c_cyan, sc?sc:"", c_white);
				}

				free(tot_str);

				if (persistent_did_reconnect)
				{
					str_add(&line, " %sC%s", c_magenta, c_white);
					persistent_did_reconnect = 0;
				}

				if (show_Bps)
				{
					str_add(&line, " %dKB/s", (int)Bps / 1024);
					if (show_bytes_xfer)
						str_add(&line, " %dKB", (int)(bytes_transferred / 1024));
					if (ask_compression)
					{
						str_add(&line, " (");
						if (!is_compressed)
							str_add(&line, gettext("not "));
						str_add(&line, gettext("compressed)"));
					}
				}

				if (use_ssl && show_fp && fp != NULL)
				{
					str_add(&line, " %s", fp);
				}

				if (verbose > 0 && their_ts > 0)
				{
					/*  if diff_ts > 0, then their clock is running too fast */
					str_add(&line, gettext(" toff=%d"), (int)toff_diff_ts);
				}

				if (verbose > 0 && age > 0)
					str_add(&line, gettext(" age=%d"), age);

				str_add(&line, "%s", c_normal);

				if (audible)
				{
#ifdef NC
					my_beep();
#else
					putchar('\a');
#endif
				}

#ifdef TCP_TFO
				if (tfo_success)
					str_add(&line, " F");
#endif

#ifdef NC
				if (ncurses_mode)
				{
					if (dummy_ms >= show_slow_log)
						slow_log("\n%s", line);
					else
						fast_log("\n%s", line);
				}
				else
#endif
				{
					printf("%s\n", line);
				}

				do_aggregates(t_total.cur, (int)(get_ts() - started_at), n_aggregates, aggregates, verbose, show_ts);

				free(line);
			}

			if (show_statuscodes && ok_str != NULL && sc != NULL)
			{
				scdummy = strchr(sc, ' ');
				if (scdummy) *scdummy = 0x00;

				if (strstr(ok_str, sc) == NULL)
				{
					ok--;
					err++;
				}
			}

			if (got_sigquit && !quiet && !machine_readable && !nagios_mode && !json_output)
			{
				got_sigquit = 0;
				stats_line(0, complete_url, count, curncount, err, ok, started_at, verbose, &t_total, avg_httping_time, show_Bps ? &bps : NULL);
			}

			break;
		}

		emit_statuslines(get_ts() - started_at);
#ifdef NC
		if (ncurses_mode)
			update_stats(&t_resolve, &t_connect, &t_request, &t_total, &t_ssl, curncount, err, sc, fp, use_tfo, nc_graph, &stats_to, &tcp_rtt_stats, re_tx, pmtu, recv_tos, &t_close, &t_write, n_dynamic_cookies, abbreviate, &stats_header_size);
#endif

		free(sc);
		free(fp);

#ifdef NC
		if (ncurses_mode)
			update_terminal();
		else
#endif
			fflush(NULL);

		if (!stop && wait > 0)
		{
			double cur_sleep = wait;

			if (adaptive_interval)
			{
				double now = get_ts();
				double interval_left = fmod(now - started_at, wait);

				if (interval_left <= 0.0)
					cur_sleep = wait;
				else
					cur_sleep = wait - interval_left;
			}

			usleep((useconds_t)(cur_sleep * 1000000.0));
		}

		reset_statst_cur(&t_resolve);
		reset_statst_cur(&t_connect);
		reset_statst_cur(&t_ssl);
		reset_statst_cur(&t_write);
		reset_statst_cur(&t_request);
		reset_statst_cur(&t_close);
		reset_statst_cur(&t_total);
		reset_statst_cur(&stats_to);
#if defined(linux) || defined(__FreeBSD__)
		reset_statst_cur(&tcp_rtt_stats);
#endif
	}

#ifdef NC
	if (ncurses_mode)
		end_ncurses();
#endif

	if (colors)
		set_colors(0);

	if (ok)
		avg_httping_time = t_total.avg / (double)t_total.n;
	else
		avg_httping_time = -1.0;

	if (!quiet && !machine_readable && !nagios_mode && !json_output)
		stats_line(1, complete_url, count, curncount, err, ok, started_at, verbose, &t_total, avg_httping_time, show_Bps ? &bps : NULL);

error_exit:
	if (nagios_mode)
		return nagios_result(ok, nagios_mode, nagios_exit_code, avg_httping_time, nagios_warn, nagios_crit);

	if (!json_output && !machine_readable)
		printf("%s", c_very_normal);

	if (json_output)
		printf("\n]\n");

	free_cookies(static_cookies, n_static_cookies);
	free_cookies(dynamic_cookies, n_dynamic_cookies);
	freeaddrinfo(ai);
	free(request);
	free(get);
	free(hostname);
	free(complete_url);

	free_headers(additional_headers, n_additional_headers);

	free(aggregates);

#ifndef NO_SSL
	if (use_ssl)
	{
		SSL_CTX_free(client_ctx);

		shutdown_ssl();
	}
#endif

	fflush(NULL);

	if (ok)
		return 0;
	else
		return 127;
}
