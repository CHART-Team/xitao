.SUFFIXES: .cxx .o

CXX = g++  -O3 -ffast-math #-DLIST

include ../loi/loikrd.defs

# options for LOI/KRD
#
ifdef LOI
LOI_SOURCES =  ../loi/loi.c ../loi/krd_common.c ../loi/krd_trace.c ../loi/krd_bench.c
LOI_OBJS    =  ../loi/loi.o ../loi/krd_common.o ../loi/krd_trace.o ../loi/krd_bench.o
LOI_DIR = ../loi
LOI_HDR = ../loi/loi.h ../loi/krd.h
else
LOI_DIR = ../loi
endif

CXXFLAGS += -I$(LOI_DIR)

.cxx.o  :
	$(CXX) -c $^ -o $@ $(LOIFLAGS)  $(CXXFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@

test: test.o  $(LOI_OBJS)
	$(CXX) $?  $(CXXFLAGS)
	numactl -C 0 ./a.out

clean:
	$(RM) *.o a.out
