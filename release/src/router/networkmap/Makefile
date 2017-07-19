include ../.config

EXEC = networkmap
OBJS = function.o networkmap.o

ifeq ($(HND_ROUTER),y)
CFLAGS += -Werror=return-type -Werror=uninitialized -Wno-date-time 
endif
CFLAGS += -Wall -DASUS -DBCMNVRAM -I. -I$(TOP)/shared -I$(TOP)/kernel_header/include -I$(SRCBASE)/include
CFLAGS  += -s -O2 -DNO_PARALLEL

ifeq ($(RTCONFIG_BWDPI),y)
CFLAGS += -I$(TOP)/sqlite -I$(TOP)/httpd
CFLAGS += -I$(TOP)/bwdpi_source/src/tdts_udb_sample
CFLAGS += -I$(TOP)/bwdpi_source/src/tdts_udb_sample/linux
CFLAGS += -I$(TOP)/bwdpi_source/include
CFLAGS += -I$(TOP)/bwdpi_source/asus_include
endif

LDFLAGS += -L$(TOP)/shared -lshared -L$(TOP)/nvram${BCMEX}${EX7} -lnvram 
ifeq ($(HND_ROUTER),y)
LDFLAGS += -L$(TOP)/wlcsm -lwlcsm
endif

ifeq ($(RTCONFIG_BCMARM),y)
LDFLAGS += -lgcc_s
endif
ifeq ($(RTCONFIG_BWDPI),y)
LDFLAGS += -L$(TOP)/bwdpi_source/asus -lbwdpi
endif
ifeq ($(RTCONFIG_QTN),y)
LDFLAGS += -L$(TOP)/libqcsapi_client -lqcsapi_client
endif

ifeq ($(RTCONFIG_NOTIFICATION_CENTER),y)
CFLAGS  += -I$(TOP)/sqlite
CFLAGS  += -I$(TOP)/nt_center/lib
CFLAGS  += -DASUSWRT_SDK
LDFLAGS += -L$(TOP)/nt_center/lib/ -lnt
LDFLAGS += -L$(TOP)/sqlite/.libs -lsqlite3
LDFLAGS += -lpthread
endif

include $(SRCBASE)/.config

#CFLAGS	+= -DDEBUG #-DDEBUG_MORE #-DDEBUG_FUNCTION
CFLAGS += -DNMP_DB
CFLAGS += -DBTN_SETUP
ifeq ($(HND_ROUTER),y)
CFLAGS += -DHND_ROUTER
endif

all: $(EXEC) asusdiscovery arpstorm

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

arpstorm: shutils.o arpstorm.o
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

asusdiscovery: shutils.o packet.o ASUS_Discovery.o ASUS_Discovery_Debug.o
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install:
	install -D $(EXEC) $(INSTALLDIR)/usr/sbin/$(EXEC)
	$(STRIP) $(INSTALLDIR)/usr/sbin/$(EXEC)
	install -D arpstorm $(INSTALLDIR)/usr/sbin/arpstorm
	$(STRIP) $(INSTALLDIR)/usr/sbin/arpstorm
	install -D asusdiscovery $(INSTALLDIR)/usr/sbin/asusdiscovery
	$(STRIP) $(INSTALLDIR)/usr/sbin/asusdiscovery

clean: 
	rm -rf *.o $(EXEC) *~ arpstorm asusdiscovery
