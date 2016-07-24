.SUFFIXES: .cxx .o

LIST_SCHEDULE:=-DOPENMP -fopenmp
#LIST_SCHEDULE:=-DTASK_GROUP
PROGRAM_OBJS=test.o 
#LIST_SCHEDULE:=-DTAO -pthread
#PROGRAM_OBJS=test.o ../tao_sched.o

include ../makefile.inc
CXX = g++ -std=c++11 -O3 -g3 -ffast-math -DLIST $(LIST_SCHEDULE)
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

CXXFLAGS += -I$(LOI_DIR)  -I..

.cxx.o  :
	$(CXX) -c $^ -o $@ $(LOIFLAGS)  $(CXXFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@

test: $(PROGRAM_OBJS) $(LOI_OBJS)
	$(CXX) $?  $(CXXFLAGS)  -ltbb 
	./a.out

clean:
	$(RM) *.o a.out taofmm $(LOI_OBJS)

