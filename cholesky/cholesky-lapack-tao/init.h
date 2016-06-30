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
} initialization_type;


static inline TYPE initializeElement(long I, long J, long i, long j);


static void zz_initializeBlock(long VN, long HN, long VBS, long HBS, long I, long J, TYPE data[VBS][HBS]) {
	for (long i=0; i < VBS; i++) {
		for (long j=0; j < HBS; j++) {
			data[i][j] = initializeElement(I, J, i, j);
		}
	}
}


static void initialize(long VN, long HN, long VBS, long HBS, TYPE data[VN/VBS][HN/HBS][VBS][HBS]) {
	for (long I = 0; I < VN; I+=VBS) {
		for (long J = 0; J < HN; J+=HBS) {
//			#pragma omp task
			zz_initializeBlock(VN, HN, VBS, HBS, I, J, data[I/VBS][J/HBS]);
		}
	}
//	#pragma omp taskwait
}


static void zz_initializeBlockPair(long N, long BS, long I, long J, TYPE data1[BS][BS], TYPE data2[BS][BS]) {
	for (long i=0; i < BS; i++) {
		for (long j=0; j < BS; j++) {
			data1[i][j] = initializeElement(I, J, i, j);
			data2[j][i] = initializeElement(J, I, j, i);
		}
	}
}


static void initializeByPairs(long N, long BS, TYPE data[N/BS][N/BS][BS][BS]) {
	for (long I = 0; I < N; I+=BS) {
		for (long J = 0; J < I; J+=BS) {
//			#pragma omp task
			zz_initializeBlockPair(N, BS, I, J, data[I/BS][J/BS], data[J/BS][I/BS]);
		}
//		#pragma omp task
		zz_initializeBlock(N, N, BS, BS, I, I, data[I/BS][I/BS]);
	}
//	#pragma omp taskwait
}


void parseInitializationType(char const *initializazionTypeString) {
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
	} else {
		fprintf(stderr, "Error: unknown initialization type parameter.\n");
		exit(1);
	}
}


static void initializeSequentially(long VN, long HN, long VBS, long HBS, TYPE data[VN/VBS][HN/HBS][VBS][HBS]) {
	for (long I = 0; I < VN; I+=VBS) {
		for (long i=0; i < VBS; i++) {
			for (long J = 0; J < HN; J+=HBS) {
				for (long j=0; j < HBS; j++) {
					data[I/VBS][J/HBS][i][j] = initializeElement(I, J, i, j);
				}
			}
		}
	}
}



static void initializeUsingUserSpecification(long VN, long HN, long VBS, long HBS, TYPE data[VN/VBS][HN/HBS][VBS][HBS]) {
	switch (initialization_type) {
		case H_INIT:
			// Horizontal panels
		case V_INIT:
			// Vertical panels
			fprintf(stderr, "Error: the blocked version of this problem does not allow H/V initialization.\n");
			exit(1);
			break;
		case B_INIT:
			// Blocks
//			#pragma omp parallel
//			#pragma omp single
			initialize(VN, HN, VBS, HBS, data);
			break;
		case PW_INIT:
			if (VN != HN) {
				fprintf(stderr, "Error: this problem does not allow pairwise initialization.\n");
				exit(1);
			}
			// Pairs of blocks around the diagonal
//			#pragma omp parallel
//			#pragma omp single
			initializeByPairs(VN, VBS, data);
			break;
		case SEQ_INIT:
			// Sequential
//			#pragma omp parallel
//			#pragma omp single
			initializeSequentially(VN, HN, VBS, HBS, data);
			break;
	}
}

void initializeUsingUserSpecification_g(long VN, long HN, long VBS, long HBS, void *data)
{ 
     initializeUsingUserSpecification(VN, HN, VBS, HBS, data);
}

#endif // USER_INITIALIZATION_BY_SHAPE_H
