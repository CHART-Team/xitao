#ifndef USER_INITIALIZATION_BY_SHAPE_H
#define USER_INITIALIZATION_BY_SHAPE_H


#include <stdlib.h>
#include <string.h>


static enum {
	H_INIT,
	V_INIT,
	B_INIT,
	PW_INIT,
	SEQ_INIT
#ifdef EXTRA_INIT_ID_STRING
	, EXTRA_INIT
#endif
} initialization_type;


static int initializationIsQuadratic() {
	return (initialization_type == B_INIT) || (initialization_type == PW_INIT);
}

static int initializationIsSequential() {
	return (initialization_type == SEQ_INIT);
}


#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
static inline TYPE initializeElement(long I, long J, long i, long j, long VN, long HN);
#else
static inline TYPE initializeElement(long I, long J, long i, long j);
#endif

#pragma omp task out(([VN]data)[0;VBS][0;HBS])
static void zz_initializeBlock(long VN, long HN, long VBS, long HBS, long I, long J, TYPE data[VN][HN]) {
	for (long i=0; i < VBS; i++) {
		for (long j=0; j < HBS; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data[i][j] = initializeElement(I, J, i, j, VN, HN);
#else
			data[i][j] = initializeElement(I, J, i, j);
#endif
		}
	}
}


static void initialize(long VN, long HN, long VBS, long HBS, TYPE data[VN][HN]) {
	for (long I = 0; I < VN; I+=VBS) {
		for (long J = 0; J < HN; J+=HBS) {
			zz_initializeBlock(VN, HN, VBS, HBS, I, J, (TYPE (*)[HN]) &data[I][J]);
		}
	}
}


static void initializeSequentially(long VN, long HN, long VBS, long HBS, TYPE data[VN][HN]) {
	for (long I = 0; I < VN; I+=VBS) {
		for (long J = 0; J < HN; J+=HBS) {
			for (long i=0; i < VBS; i++) {
				for (long j=0; j < HBS; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
					data[I+i][J+j] = initializeElement(I, J, i, j, VN, HN);
#else
					data[I+i][J+j] = initializeElement(I, J, i, j);
#endif
				}
			}
		}
	}
}


#pragma omp task out(([VN1]data1)[0;VBS1][0;HBS1], ([VN2]data2)[0;VBS2][0;HBS2])
static void zz_initialize2Blocks(long VN1, long HN1, long VBS1, long HBS1, long I1, long J1, TYPE data1[VN1][HN1], long VN2, long HN2, long VBS2, long HBS2, long I2, long J2, TYPE data2[VN2][HN2]) {
	for (long i=0; i < VBS1; i++) {
		for (long j=0; j < HBS1; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data1[i][j] = initializeElement(I1, J1, i, j, VN1, HN1);
#else
			data1[i][j] = initializeElement(I1, J1, i, j);
#endif
		}
	}
	for (long i=0; i < VBS2; i++) {
		for (long j=0; j < HBS2; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data2[i][j] = initializeElement(I2, J2, i, j, VN2, HN2);
#else
			data2[i][j] = initializeElement(I2, J2, i, j);
#endif
		}
	}
}


#pragma omp task out(([N]data1)[0;BS][0;BS], ([N]data2)[0;BS][0;BS])
static void zz_initializeBlockPair(long N, long BS, long I, long J, TYPE data1[N][N], TYPE data2[N][N]) {
	for (long i=0; i < BS; i++) {
		for (long j=0; j < BS; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data1[i][j] = initializeElement(I, J, i, j, N, N);
			data2[j][i] = initializeElement(J, I, j, i, N, N);
#else
			data1[i][j] = initializeElement(I, J, i, j);
			data2[j][i] = initializeElement(J, I, j, i);
#endif
		}
	}
}


static void initializeByPairs(long N, long BS, TYPE data[N][N]) {
	for (long I = 0; I < N; I+=BS) {
		for (long J = 0; J < I; J+=BS) {
			zz_initializeBlockPair(N, BS, I, J, (TYPE (*)[N]) &data[I][J], (TYPE (*)[N]) &data[J][I]);
		}
		zz_initializeBlock(N, N, BS, BS, I, I, (TYPE (*)[N]) &data[I][I]);
	}
}


#pragma omp task out(([M1]data1)[0;H1][0;W1], ([M2]data2)[0;H2][0;W2])
static void zz_initializeDependentPair(
	long M1, long N1, long M2, long N2,
	long I1, long J1, long I2, long J2,
	long H1, long W1, long H2, long W2,
	TYPE data1[M1][N1], TYPE data2[M2][N2]
) {
	for (long i=0; i < H1; i++) {
		for (long j=0; j < W1; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data1[i][j] = initializeElement(I1, J1, i, j, M1, N1);
#else
			data1[i][j] = initializeElement(I1, J1, i, j);
#endif
		}
	}
	
	for (long i=0; i < H2; i++) {
		for (long j=0; j < W2; j++) {
#ifdef PASS_DIMENSIONS_TO_INITIALIZATION_FUNCTION
			data2[i][j] = initializeElement(I2, J2, i, j, M2, N2);
#else
			data2[i][j] = initializeElement(I2, J2, i, j);
#endif
		}
	}
}


static void parseInitializationType(char const *initializazionTypeString) {
	if (strcmp(initializazionTypeString, "H") == 0) {
		initialization_type = H_INIT;
	} else if (strcmp(initializazionTypeString, "V") == 0) {
		initialization_type = V_INIT;
	} else if (strcmp(initializazionTypeString, "B") == 0) {
		initialization_type = B_INIT;
	} else if (strcmp(initializazionTypeString, "PW") == 0) {
		initialization_type = PW_INIT;
	} else if (strcmp(initializazionTypeString, "SEQ") == 0) {
		initialization_type = SEQ_INIT;
#ifdef EXTRA_INIT_ID_STRING
	} else if (strcmp(initializazionTypeString, EXTRA_INIT_ID_STRING) == 0) {
		initialization_type = EXTRA_INIT;
#endif
	} else {
		fprintf(stderr, "Error: unknown initialization type parameter.\n");
		exit(1);
	}
}


static void initializeUsingUserSpecification(long VN, long HN, long VBS, long HBS, TYPE data[VN][HN]) {
	switch (initialization_type) {
		case H_INIT:
			// Horizontal panels
			initialize(VN, HN, VBS, HN, data);
			break;
		case V_INIT:
			// Vertical panels
			initialize(VN, HN, VN, HBS, data);
			break;
		case B_INIT:
			// Blocks
			initialize(VN, HN, VBS, HBS, data);
			break;
		case PW_INIT:
			if (HN != VN) {
				fprintf(stderr, "Error: this problem does not allow pairwise initialization.\n");
				exit(1);
			}
			// Pairs of blocks around the diagonal
			initializeByPairs(HN, HBS, data);
			break;
		case SEQ_INIT:
			// Sequential
			initializeSequentially(VN, HN, VN, HN, data);
			break;
#if EXTRA_INIT_HAS_FUNCTION
		case EXTRA_INIT:
			// Problem-specific initialization
			initializeExtra(VN, HN, VBS, HBS, data);
			break;
#endif
	}
}


static int usesExtraInitialization() {
#ifdef EXTRA_INIT_ID_STRING
	return (initialization_type == EXTRA_INIT);
#else
	return 0;
#endif
}


#endif // USER_INITIALIZATION_BY_SHAPE_H
