#ifndef BLAS_H
#define BLAS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef F77_FUNC
#error This code needs to have F77_FUNC defined. Check that configure.ac is correct.
#endif


#define lapack_dgemm F77_FUNC(dgemm,DGEMM)
#define lapack_dgemv F77_FUNC(dgemv,DGEMV)
#define lapack_dpotrf F77_FUNC(dpotrf,DPOTRF)
#define lapack_dtrsm F77_FUNC(dtrsm,DTRSM)
#define lapack_dtrsv F77_FUNC(dtrsv,DTRSV)
#define lapack_dsyrk F77_FUNC(dsyrk,DSYRK)
#define lapack_dgetrf F77_FUNC(dgetrf,DGETRF)

#define lapack_dlaswp F77_FUNC(dlaswp,DLASWP)

#define lapack_idamax F77_FUNC(idamax,IDAMAX)
#define lapack_dcopy F77_FUNC(dcopy,DCOPY)
#define lapack_daxpy F77_FUNC(daxpy,DAXPY)
#define lapack_dasum F77_FUNC(dasum,DASUM)

void lapack_dgemm(
	char const *transA, char const *transB,
	const integer *m, const integer *n, const integer *k,
	const double *alpha,
	const double *a, const integer *lda,
	const double *b, const integer *ldb,
	const double *beta,
	double *c, const integer *ldc
);
void lapack_dgemv(
	char const *trans,
	const integer *m, const integer *n,
	const double *alpha,
	const double *a, const integer *lda,
	const double *x, const integer *incx,
	const double *beta,
	double *y, const integer *incy
);
void lapack_dpotrf(
	char const *uplo,
	const integer *n,
	double *a, const integer *lda,
	integer *info
);
void lapack_dtrsm(
	const char *side, const char *uplo, const char *transa, const char *diag,
	const integer *m, const integer *n,
	const double *alpha,
	const double *a, const integer *lda,
	double *b, const integer *ldb
);
void lapack_dtrsv(
	const char *uplo, const char *trans, const char *diag,
	const integer *n,
	const double *a, const integer *lda,
	double *x, const integer *incx
);
void lapack_dsyrk(
	const char *uplo, const char *trans,
	const integer *n, const integer *k,
	const double *alpha,
	const double *a, const integer *lda,
	const double *beta,
	double *c, const integer *ldc
);
void lapack_dgetrf(
	const integer *m, const integer *n,
	double *a, const integer *lda,
	integer *ipiv,
	integer *info
);

void lapack_dlaswp(
	const integer *n,
	double *a, const integer *lda,
	const integer *k1, const integer *k2,
	const integer *ipiv, const integer *incx
);

integer lapack_idamax(
	const integer *n,
	const double *x, const integer *incx
);
void lapack_dcopy(
	const integer *N, 
	const double *X, const integer *incX,
	double *Y, const integer *incY
);
void lapack_daxpy(
	const integer *N, 
	const double *alpha, 
	const double *X, const integer *incX, 
	double *Y, const integer *incY
);
double lapack_dasum(
	const integer *n,
	const double *x, const integer *incx
);

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) < (y) ? (y) : (x))


#endif /* BLAS_H */
