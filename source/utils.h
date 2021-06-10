/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

double get_ts(void);

void split_string(const char *in, const char *split, char ***list, int *list_n);
void free_splitted_string(char **list, int n);

void str_add(char **to, const char *what, ...);

#define GIGA 1000000000.0
#define MEGA 1000000.0
#define KILO 1000.0

char * format_value(double value, int digits_sig, int digits_nsig, char abbreviate);

#define min(x, y)	((x) < (y) ? (x) : (y))
#define max(x, y)	((x) > (y) ? (x) : (y))
