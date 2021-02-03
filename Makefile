# %: wildcard
# $(var:suffix=replacement) === $(patsubst %suffix,%replacement,$(var))
# $(hello_src:.cpp=.o) -> this extracts hello.o from hello.cpp

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
	-Wcast-align \
	-Wcast-qual \
	-Wdisabled-optimization \
	-Wformat=2 \
	-Winit-self \
	-Wlogical-op \
	-Wmissing-declarations \
	-Wmissing-include-dirs \
	-Wredundant-decls \
	-Wshadow \
	-Wsign-conversion \
	-Wstrict-overflow=5 \
	-Wswitch-default \
	-Wundef \
	-Werror \
	-Wno-unused

all: $(target) post-target

# $(info $(target)_src: $($(target)_src))
# $(info $(target)_objs: $($(target)_objs))
# $(info $(target)_deps: $($(target)_deps))

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

# rule for generating obj files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(target): $($(target)_objs)
	$(CC) $(CFLAGS) -o $(target) $($(target)_objs)

post-target: $(target)
	mkdir -p .objs
	mv *.o *.d .objs

include $($(target)_deps) # include all dep files in the makefile

clean:
	rm -rf .objs *d $(target) > /dev/null 2>&1

.PHONY: clean all