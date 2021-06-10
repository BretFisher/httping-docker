/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#ifndef __GEN_H__
#define __GEN_H__

#define RC_OK		0
#define RC_SHORTREAD	-1
#define RC_SHORTWRITE	-1
#define RC_TIMEOUT	-2
#define RC_CTRLC	-3
#define RC_INVAL	-4

#define RECV_BUFFER_SIZE (128 * 1024)

#define SPAM_FILE "/tmp/httping.dat"

#define MAX_SHOW_SUPPRESSION 3

#ifdef NO_SSL
	#define SSL	void
	#define SSL_CTX	void
	#define BIO	void
#endif

#define PI (4 * atan(1.0))
#define MY_DOUBLE_INF	999999999999999.9

#ifdef TCP_TFO
	#ifndef MSG_FASTOPEN
		#define MSG_FASTOPEN	0x20000000
	#endif

	#ifndef TCP_FASTOPEN
		#define TCP_FASTOPEN	23
	#endif
	#ifndef TCPI_OPT_SYN_DATA
		#define TCPI_OPT_SYN_DATA	32
	#endif
#endif

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef struct
{
	double cur, min, avg, max, sd;
	int n;
	char valid, cur_valid;
} stats_t;

int enc_b64(char *source, int source_lenght, char *target);

void init_statst(stats_t *data);
void update_statst(stats_t *data, double in);
void reset_statst_cur(stats_t *data);
double calc_sd(stats_t *in);

#endif
