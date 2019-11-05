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

include ../../makefile.sched
CXXFLAGS += -I../../include -I.
CXXFLAGS += -O2

CXXFLAGS += -fopenmp --std=c++11 -fno-unroll-loops ${EXTRAEINC}

LIBS = -fopenmp -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
PROGRAM_OBJS=dataparallel.o 

all: dataparallel

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

dataparallel: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: dataparallel 
	./dataparallel 1000 4 0 64

clean:
	rm -rf dataparallel $(PROGRAM_OBJS) html latex
