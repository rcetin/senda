CC=gcc
target = senda

$(target)_src = main.c \
				eth/ethersend.c \
				debug/debug.c \
				utils/utils.c \
				udp/udpsend.c \
				tcp/tcpsend.c

$(target)_inc = -I$(shell pwd)

$(target)_objs = $($(target)_src:.c=.o)

$(target)_deps = $($(target)_objs:.o=.d) # one dependency file for each source

CFLAGS = -std=gnu11 \
	-g3 \
	-pedantic \
	-Wall \
	-Wextra \
	-Wfloat-equal \
	-Wundef \
	-Wshadow \
	-Wcast-align \
	-Wpointer-arith \
	-Wstrict-prototypes \
	-Wcast-qual \
	-Wdisabled-optimization \
	-Wformat=2 \
	-Winit-self \
	-Wlogical-op \
	-Wmissing-declarations \
	-Wmissing-include-dirs \
	-Wredundant-decls \
	-Wsign-conversion \
	-Wstrict-overflow=5 \
	-Wswitch-default \
	-Werror \
	-Wno-unused

all: $(target) post-target

%.d: %.c
	@$(CC) $(CFLAGS) $($(target)_inc) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) $(CFLAGS) $($(target)_inc) -c $< -o $@

$(target): $($(target)_objs)
	$(CC) $(CFLAGS) -o $(target) $($(target)_objs)

post-target: $(target)
	mkdir -p .build
	find . -name '*.d' -exec mv -v {} .build \;
	find . -name '*.o' -exec mv -v {} .build \;

include $($(target)_deps) # include all dep files in the makefile

clean:
	rm -rf .build *d $(target) > /dev/null 2>&1

.PHONY: clean all