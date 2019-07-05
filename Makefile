BIN=orb2ecms

ldlibs= $(ORBLIBS) $(STOCKLIBS)

#include $(ANTELOPEMAKE)  
include /opt/antelope/5.7/include/antelopemake1

DIRS=

OBJS=orb2ecms.o \
	site_read.o \
	sock_func.o

orb2ecms : $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

force:

