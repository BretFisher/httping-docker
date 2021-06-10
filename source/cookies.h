void add_cookie(char ***cookies, int *n_cookies, char *in);
void combine_cookie_lists(char ***destc, int *n_dest, char **src, int n_src);
void free_cookies(char **dynamic_cookies, int n_dynamic_cookies);
void get_cookies(const char *headers, char ***dynamic_cookies, int *n_dynamic_cookies, char ***static_cookies, int *n_static_cookies);
