CC=gcc
target = senda

$(target)_src = main.c \
				ethersend.c \
				debug/debug.c \
				utils/utils.c

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
	-Wconversion \
	-Wswitch-default \
	-Werror \
	-Wno-unused

all: $(target) post-target

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(target): $($(target)_objs)
	$(CC) $(CFLAGS) -o $(target) $($(target)_objs)

post-target: $(target)
	mkdir -p .build
	mv *.o *.d $(target) .build

include $($(target)_deps) # include all dep files in the makefile

clean:
	rm -rf .build *d $(target) > /dev/null 2>&1

.PHONY: clean all