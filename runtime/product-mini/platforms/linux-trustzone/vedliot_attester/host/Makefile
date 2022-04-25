ifdef PROFILING
	PROFILING_FLAG = -DPROFILING_$(PROFILING)
endif

CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objedump
READELF ?= $(CROSS_COMPILE)readelf

OBJS = main.o

# TODO: TEEC_EXPORT needs to be specified
CFLAGS += -O3 -Wall -I../ta/include -I$(TEEC_EXPORT)/include -I./include $(PROFILING_FLAG)
# Add/link other required libraries here
LDADD += -lteec -Wl,-L$(TEEC_EXPORT)/lib

BINARY = vedliot_attester

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $< $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
