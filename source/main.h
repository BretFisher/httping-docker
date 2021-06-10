extern volatile int stop;
extern volatile char got_sigquit;
extern int max_x, max_y;

void determine_terminal_size(int *max_y, int *max_x);
void handler(int sig);
