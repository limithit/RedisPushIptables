
# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -shared
else
	SHOBJ_CFLAGS ?= -W -Wall -dynamic -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
endif

.SUFFIXES: .c .so .xo .o

all: iptablespush.so ttl_iptables

ttl_iptables: ttl_iptables.c
		$(CC) ttl_iptables.c -o $@ `pkg-config --cflags-only-I hiredis` `pkg-config --libs-only-l hiredis`

.c.xo:
	$(CC) -I. $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

iptablespush.xo: ./redismodule.h

iptablespush.so: iptablespush.xo
	$(LD) -o $@ $< $(SHOBJ_LDFLAGS) $(LIBS) -lc

clean:
	rm -rf *.xo *.so ttl_iptables

install:
	 cp ttl_iptables /usr/sbin/
	 cp init.d/ttl_iptables /etc/init.d
