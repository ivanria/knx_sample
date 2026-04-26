
PROG		:= knx_task
DEBUG		:= 1
SOURCES		:= $(wildcard *.c)
OBJECTS		:= $(patsubst %c,%o,$(SOURCES))
DEPFILES	:= $(patsubst %.c,%.d,$(SOURCES))
CFLAGS		:= -Wall -Wextra -Wpedantic -D_GNU_SOURCE

ifeq ($(DEBUG), 1)
		CFLAGS += -g -DDEBUG
		LDFLAGS := -g
else
		CFLAGS += -O2
endif



$(PROG): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<


ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILES)
endif

.PHONY: all
all: $(PROG)
	$(MAKE) $(PROG)

.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(PROG) $(DEPFILES)


