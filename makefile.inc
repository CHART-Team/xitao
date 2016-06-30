
EXTRAEINC=  # -DEXTRAE -I/home/miquelp/local/include
EXTRAELIBS= # -L/home/miquelp/local/lib -lpttrace -Wl,-rpath=/home/miquelp/local/lib

TBBLIB := ${HOME}/local/lib/inteltbb/
TBBINC := ${HOME}/local/include
TBBLFLAGS := -std=c++11 -DTBB -ltbb -L$(TBBLIB) -I$(TBBINC) -Wl,-rpath=$(TBBLIB)  

QTHLIB := /home/miquelp/parallel2/sys/inst/g/lib
QTHINC := /home/miquelp/parallel2/sys/inst/g/include
QTHLFLAGS := -std=c++11 -DQTHREAD -lqthread -L$(QTHLIB) -I$(QTHINC) -Wl,-rpath=$(QTHLIB)


