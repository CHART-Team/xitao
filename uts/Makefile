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
CFLAGS := $(LIBS) -g3 -I.. -I.  -O3 ${EXTRAEINC}
CXXFLAGS=$(CFLAGS) --std=c++11  -pthread ${EXTRAEINC}

include makefile.uts

LIBS = -lm ${EXTRAELIBS}
PROGRAM_OBJS=uts.o brg_sha1.o tao_main.o bots_common.o ../tao_sched.o


CFLAGS += -DTAO_UTS1_WIDTH=$(LEV1)
CFLAGS += -DTAO_UTS2_WIDTH=$(LEV2)
CFLAGS += -DUTS_BLOCK_SIZE=$(BLOCK_SIZE)

all: uts

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
 
%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@
 

uts: $(PROGRAM_OBJS)
	$(CXX) $^ -o $@ $(CXXFLAGS) ${LIBS} -L$(TBBLIB) -ltbbmalloc_proxy -ltbbmalloc -Wl,-rpath=$(TBBLIB)

clean:
	rm $(PROGRAM_OBJS)


