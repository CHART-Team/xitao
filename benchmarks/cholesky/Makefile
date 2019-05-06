# Makefile for cholesky 
# This just attempts to reproduce the same steps as the Deps benchmark suite

OMP_CHOLESKY=cholesky-lapack-omp
OMPSS_CHOLESKY=cholesky-lapack-ompss
TAO_CHOLESKY=cholesky-lapack-tao
###################################################
#                                                 #
#                  CHOLESKY TAO                   #
#                                                 #
###################################################

include ../../makefile.hardware
include ../../makefile.sched
include makefile.chol
XITAOROOT = ../..
XITAOLIB  = $(XITAOROOT)/lib
CFLAGS+= -I$(XITAOROOT)/include
CXXFLAGS+= -I$(XITAOROOT)/include
LIBS += -L$(XITAOLIB) -lxitao -llapack -lrefblas -lgfortran 

CFLAGS   += -std=gnu99 -DHAVE_CONFIG_H -I.  -Dinteger=int32_t -g -O2 -fpermissive ${EXTRAEINC} 
CXXFLAGS += -std=c++11 -DHAVE_CONFIG_H -I.  -Dinteger=int32_t -g -O2 -fpermissive ${EXTRAEINC}

# this will probably be added later
#include makefile.cholesky

LIBS += -lm ${EXTRAELIBS}
PROGRAM_OBJS=${TAO_CHOLESKY}/cholesky-tao.o ${TAO_CHOLESKY}/cholesky.o 

cholesky-tao: ${TAO_CHOLESKY}/cholesky-tao.cxx  ${TAO_CHOLESKY}/cholesky.c 
	$(CC)  ${CFLAGS} -I.. -c ${TAO_CHOLESKY}/cholesky.c -o ${TAO_CHOLESKY}/cholesky.o
	$(CXX) ${CXXFLAGS} -I.. -c ${TAO_CHOLESKY}/cholesky-tao.cxx -o ${TAO_CHOLESKY}/cholesky-tao.o
	$(CXX) -pthread -o $@ ${PROGRAM_OBJS} ${LIBS} 

CC=gcc
CXX=g++
CFLAGS=-std=gnu99 -DHAVE_CONFIG_H -I.  -Dinteger=int32_t -g -O2

# CHOLESKY SERIAL

SFLAGS=-MT $(OMP_CHOLESKY)/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.o -MD -MP -MF ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Tpo
SOBJS=${OMP_CHOLESKY}/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.o 

cholesky-serial: ${OMP_CHOLESKY}/cholesky.c
	gcc ${CFLAGS} ${SFLAGS}  -c -o ${SOBJS} $<
	mv -f ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Tpo ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Po
	gcc ${CFLAGS}  -o $@ ${SOBJS} -lm 

# CHOLESKY OPENMP

OMPFLAGS=-MT $(OMP_CHOLESKY)/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.o -MD -MP -MF ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Tpo
OMPOBJS=${OMP_CHOLESKY}/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.o

cholesky-omp: ${OMP_CHOLESKY}/cholesky.c
	gcc ${CFLAGS} -fopenmp ${OMPFLAGS}  -c -o  ${OMPOBJS} $<
	mv -f ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Tpo ${OMP_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_omp-cholesky.Po
	gcc ${CFLAGS} -fopenmp  -o $@  ${OMPOBJS}  -lm ${AMD_ACML}

# CHOLESKY OMPSS

OMPSSFLAGS=-MT ${OMPSS_CHOLESKY}/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.o -MD -MP -MF ${OMPSS_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.Tpo
OMPSSOBJS=${OMPSS_CHOLESKY}/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.o

cholesky-ompss: ${OMPSS_CHOLESKY}/cholesky.c
	smpcc --ompss ${CFLAGS} ${OMPSSFLAGS}  -c -o ${OMPSSOBJS} $<   
	mv -f ${OMPSS_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.Tpo ${OMPSS_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.Po
	smpcc --ompss  ${CFLAGS} -o $@ ${OMPSSOBJS} -lm ${AMD_ACML} 

cholesky-ompss-instrument: ${OMPSS_CHOLESKY}/cholesky.c
	smpcc --ompss --instrument ${CFLAGS} ${OMPSSFLAGS}  -c -o ${OMPSSOBJS}  $< 
	mv -f ${OMPSS_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.Tpo ${OMPSS_CHOLESKY}/.deps/blocks_cholesky_lapack_cholesky_lapack_blocks_ompss-cholesky.Po
	smpcc --ompss --instrument ${CFLAGS} -o $@ ${OMPSSOBJS} -lm ${AMD_ACML} 


clean: 
	rm -f cholesky-serial cholesky-omp cholesky-ompss cholesky-ompss-instrument cholesky-tao cholesky-tao-extrae ${SOBJS} ${OMPOBJS} ${OMPSSOBJS} 
