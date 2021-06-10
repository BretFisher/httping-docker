/* #define _TEST */

#include <math.h>
#ifdef _TEST
#include <stdio.h>
#include <stdlib.h>
#endif

double x_est_last = 0.0, P_last = 0.0, Q = 0.0, R = 0.0, K = 0.0, P = 0.0, P_temp = 0.0, x_temp_est = 0.0, x_est = 0.0, z_measured = 0.0, z_real = 0.0, sum_error_kalman = 0.0, sum_error_measure = 0.0;
char first = 1;

void kalman_init(double ideal_value)
{
	/* initial values for the kalman filter */
	x_est_last = 0;
	P_last = 0;
	/* the noise in the system (FIXME?) */
	Q = 0.022;
	R = 0.617;
	z_real = ideal_value; /* 0.5;  the ideal value we wish to measure */

	first = 1;
}

double kalman_do(double z_measured)
{
	/* initialize with a measurement */
	if (first)
	{
		first = 0;
		x_est_last = z_measured;
	}

	/* do a prediction */
	x_temp_est = x_est_last;
	P_temp = P_last + Q;
	/* calculate the Kalman gain */
	K = P_temp * (1.0/(P_temp + R));
	/* measure */
	/*z_measured = z_real + frand()*0.09; the real measurement plus noise*/
	/* correct */
	x_est = x_temp_est + K * (z_measured - x_temp_est); 
	P = (1- K) * P_temp;
	/* we have our new system */

#ifdef _TEST
	printf("Ideal    position: %6.3f \n",z_real);
	printf("Mesaured position: %6.3f [diff:%.3f]\n",z_measured,fabs(z_real-z_measured));
	printf("Kalman   position: %6.3f [diff:%.3f]\n",x_est,fabs(z_real - x_est));
#endif

	sum_error_kalman += fabs(z_real - x_est);
	sum_error_measure += fabs(z_real-z_measured);

	/* update our last's */
	P_last = P;
	x_est_last = x_est;

#ifdef _TEST
	printf("Total error if using raw measured:  %f\n",sum_error_measure);
	printf("Total error if using kalman filter: %f\n",sum_error_kalman);
	printf("Reduction in error: %d%% \n",100-(int)((sum_error_kalman/sum_error_measure)*100));
#endif

	return x_est;
}

#ifdef _TEST
int main(int argc, char *argv[])
{
	kalman_init(0.0);

	for(int loop=0; loop<25; loop++)
	{
		double v = drand48();
		printf("%d] %f %f\n", loop + 1, v, kalman_do(v));
	}

	return 0;
}
#endif
