/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

void error_exit(const char *format, ...);
void set_error(const char *str, ...);
void clear_error(void);
char * get_error(void);
