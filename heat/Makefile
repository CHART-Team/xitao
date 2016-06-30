##############################################################################################
#  This program is part of the Barcelona OpenMP Tasks Suite                                  #
#  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  #
#  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   #
#                                                                                            #
#  This program is free software; you can redistribute it and/or modify                      #
#  it under the terms of the GNU General Public License as published by                      #
#  the Free Software Foundation; either version 2 of the License, or                         #
#  (at your option) any later version.                                                       #
#                                                                                            #
#  This program is distributed in the hope that it will be useful,                           #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                            #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             #
#  GNU General Public License for more details.                                              #
#                                                                                            #
#  You should have received a copy of the GNU General Public License                         #
#  along with this program; if not, write to the Free Software                               #
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            #
##############################################################################################

include ../makefile.inc
CFLAGS := $(LIBS) -g -I.. -I. -O3 ${EXTRAEINC}
CXXFLAGS=$(CFLAGS)  -pthread ${EXTRAEINC}

include makefile.heat

CC=gcc
CXX=g++
LIBS = -lm ${EXTRAELIBS}
PROGRAM_OBJS=heat-tao.o misc.o ../tao_sched.o

LOIFLAGS :=


include ../loi/loikrd.defs

# options for LOI/KRD
ifdef LOI
LOI_SOURCES =  ../loi/loi.c ../loi/krd_common.c ../loi/krd_trace.c ../loi/krd_bench.c
LOI_OBJS    =  ../loi/loi.o ../loi/krd_common.o ../loi/krd_trace.o ../loi/krd_bench.o
LOI_DIR = ../loi
LOI_HDR = ../loi/loi.h ../loi/krd.h
else
LOI_DIR = ../loi
endif

CXXFLAGS += -I$(LOI_DIR)
all: heat-tao

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS)  $(LOIFLAGS) --std=gnu11 -c $< -o $@
 
%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS)  $(LOIFLAGS) --std=c++11  -c $< -o $@
 
heat-tao: $(PROGRAM_OBJS)  $(LOI_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) ${LIBS} -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB)

heat: heat.o solver.o misc.o
	$(CC) $^ -o $@ $(CFLAGS) ${LIBS}

clean:
	rm $(PROGRAM_OBJS)

