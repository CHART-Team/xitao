#ifndef BLAS_INIT_H
#define BLAS_INIT_H

void inline force_blas_init(void) {
	const integer n = 128;
	double const alpha = 1.0;
	double const beta = 1.0;
	const integer one = 1;
	
	double *a = malloc(sizeof(double) * n * n);
	double *b = malloc(sizeof(double) * n * n);
	double *c = malloc(sizeof(double) * n * n);
	
	for (int i=0; i < n*n; i++) {
		a[i] = 1;
		b[i] = 2;
		c[i] = i % (n - 1);
	}
	dgemm_("N", "N", &n, &n, &n, &alpha, a, &n, b, &n, &beta, c, &n);
	free(a);
	free(b);
	free(c);
}

#endif /* BLAS_INIT_H */
