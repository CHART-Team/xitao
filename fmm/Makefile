.SUFFIXES: .cxx .o


include makefile.fmm

include ../makefile.inc
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
	$(CXX) -c $^ -o $@ $(LOIFLAGS)  $(CXXFLAGS) $(TBBLFLAGS) 

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@

test: $(PROGRAM_OBJS) $(LOI_OBJS)
	$(CXX) $?  $(CXXFLAGS) $(TBBLFLAGS) 
	./a.out

clean:
	$(RM) *.o a.out taofmm $(PROGRAM_OBJS) $(LOI_OBJS)

