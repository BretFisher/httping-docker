#include <fftw3.h>

int main(int argc, char *argv[])
{
	/* note: it only needs to compile */
	void *dummy = fftw_malloc(4096);
	double *dummyd = (double *)dummy;

	void *plan = fftw_plan_dft_r2c_1d(12345, dummyd, NULL, FFTW_ESTIMATE);

	return 0;
}
