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

CXXFLAGS += -I../../include 
CXXFLAGS += -O0
CXXFLAGS += --std=c++11  ${EXTRAEINC}

CXXFLAGS += -DDOUBLE
#CXXFLAGS += -DSINGLE

LIBS = -lm ${EXTRAELIBS} -L../../lib -lxitao -pthread
SYNTH_OBJS=synthdag.o  

all: synbench

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

synbench: $(SYNTH_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

chains: $(CHAINS_OBJS)
	$(CXX) $^ -o $@ ${LIBS}

run: all 
	./synbench 64 4 5000 1000 4

clean:
	rm -rf synbench $(SYNTH_OBJS)  graph.txt 


