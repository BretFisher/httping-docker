void fft_init(int sample_rate_in);
void fft_free(void);
void fft_do(double *in, double *output_mag, double *output_phase);
void fft_stop(void);
