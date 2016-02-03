# Makefile for compiling the KRD tools

include loikrd.defs

CXX = g++ -O3
LFLAGS += -lrt

all: tools
clean: clean_tools

accum_tool: krd_common.c  krd.c 
	$(CXX) $^  -o $@  -DBUILD_ACCUM_TOOL -I../include  -lm $(LFLAGS)

coherence_tool: krd_common.c krd_trace.c  
	$(CXX) $^  -o $@  -DBUILD_COHERENCE_TOOL -I../include  -lm $(LFLAGS)

krd_tool: krd_common.c krd.c krd_trace.c 
	$(CXX) $^  -o $@  -DBUILD_HISTO_TOOL -I../include  -lm $(LFLAGS)

merge_tool: krd_common.c krd_trace.c 
	$(CXX) $^  -o $@  -DBUILD_MERGE_TOOL -I../include  -lm $(LFLAGS)

intlv_tool: krd_common.c krd_trace.c 
	$(CXX) $^  -o $@  -DBUILD_INTERLEAVE_TOOL -I../include -lm $(LFLAGS)

dump_tool: krd_common.c krd_trace.c 
	$(CXX) $^  -o $@  -DBUILD_DUMP_TOOL -I../include -lm $(LFLAGS)

partition_tool: krd_common.c krd_trace.c
	$(CXX) $^  -o $@  -DBUILD_CUT_TOOL -I../include -lm $(LFLAGS)

gentrace_tool: gentrace.c
	$(CXX) $^  -o $@  -DBUILD_GENTRACE_TOOL -I../include -lm $(LFLAGS)

clean_tools:
	rm -f accum_tool  merge_tool intlv_tool krd_tool coherence_tool partition_tool dump_tool gentrace_tool

tools: accum_tool krd_tool intlv_tool merge_tool coherence_tool partition_tool dump_tool gentrace_tool



