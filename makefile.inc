
TBBLIB := /usr/lib
TBBINC := /usr/include/tbb
TBBLFLAGS := -std=c++11 -DTBB -ltbb -L$(TBBLIB) -I$(TBBINC) -Wl,-rpath=$(TBBLIB)  

QTHLIB := /home/miquel/src-git/parallel2/sys/inst/g/lib
QTHINC := /home/miquel/src-git/parallel2/sys/inst/g/include/
QTHLFLAGS := -std=c++11 -DQTHREAD -lqthread -L$(QTHLIB) -I$(QTHINC) -Wl,-rpath=$(QTHLIB)  
