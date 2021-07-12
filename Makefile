CC := gcc
target := senda
OBJ_PATH := ./build/


########################### MAIN ################################
$(target)_src = eth/ethersend.c \
				debug/debug.c \
				utils/utils.c \
				udp/udpsend.c \
				tcp/tcpsend.c \
				config/config.c \
				config/json/json_parser.c \
				main.c

$(target)_inc = -I$(shell pwd)
OBJS := $(patsubst %,$(OBJ_PATH)%,$($(target)_src:.c=.o))
DEPS := $(patsubst %,$(OBJ_PATH)%,$($(target)_src:.c=.d))
###########################################################

CFLAGS = -std=gnu11 \
	-g3 \
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
	-Wstrict-overflow=5 \
	-Wswitch-default \
	-Werror \
	-Wno-unused

LDFLAGS = -ljson-c \
			-lpthread \
			-lcmocka

all: start $(target) post-target

$(OBJ_PATH)%.d: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $($(target)_inc) $< -MM -MT $(@:.d=.o) >$@

$(OBJ_PATH)%.o: %.c
	@mkdir -p $(@D)
	$(info [Build] $@)
	@$(CC) $(CFLAGS) $($(target)_inc) -c $< -o $@

$(target): start $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(target) $(LDFLAGS)

post-target: $(target)
	$(info [Generate] $(target).)
	@echo "Done."

clean: start clean-test
	$(info [Cleaning])
	@rm -rf build $(target)

start:
	$(info ==================)
	$(info [Target] $(target))
	$(info ==================)

.PHONY: clean all

-include $(DEPS)
include Makefile.test