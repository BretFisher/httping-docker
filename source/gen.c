#include <math.h>
#include <string.h>

#include "gen.h"

void init_statst(stats_t *data)
{
	memset(data, 0x00, sizeof(stats_t));

	data -> min = MY_DOUBLE_INF;
	data -> max = -data -> min;
}

void update_statst(stats_t *data, double in)
{
	data -> cur = in;
	data -> min = min(data -> min, in);
	data -> max = max(data -> max, in);
	data -> avg += in;
	data -> sd += in * in;
	(data -> n)++;
	data -> valid = 1;
	data -> cur_valid = 1;
}

void reset_statst_cur(stats_t *data)
{
	data -> cur_valid = 0;
}

double calc_sd(stats_t *in)
{
	double avg = 0.0;

	if (in -> n == 0 || !in -> valid)
		return 0;

	avg = in -> avg / (double)in -> n;

	return sqrt((in -> sd / (double)in -> n) - pow(avg, 2.0));
}

/* Base64 encoding start */  
const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_tryptique(char source[3], char result[4])
/* Encode 3 char in B64, result give 4 Char */
 {
    int tryptique, i;
    tryptique = source[0];
    tryptique *= 256;
    tryptique += source[1];
    tryptique *= 256;
    tryptique += source[2];
    for (i=0; i<4; i++)
    {
 	result[3-i] = alphabet[tryptique%64];
	tryptique /= 64;
    }
} 

int enc_b64(char *source, int source_lenght, char *target)
{
	/* Divide string /3 and encode trio */
	while (source_lenght >= 3) {
		encode_tryptique(source, target);
		source_lenght -= 3;
		source += 3;
		target += 4;
	}
	/* Add padding to the rest */
	if (source_lenght > 0) {
		char pad[3];
	 	memset(pad, 0, sizeof pad);
		memcpy(pad, source, source_lenght);
		encode_tryptique(pad, target);
		target[3] = '=';
		if (source_lenght == 1) target[2] = '=';
		target += 4;
	}
	target[0] = 0;
	return 1;
} 
/* Base64 encoding END */
