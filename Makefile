
PROG		:= knx_sample
DEBUG		:= 1
SOURCES		:= $(wildcard *.c)
OBJECTS		:= $(patsubst %c,%o,$(SOURCES))
DEPFILES	:= $(patsubst %.c,%.d,$(SOURCES))
CFLAGS		:= -Wall -Wextra -Wpedantic -D_GNU_SOURCE

ifeq ($(DEBUG), 1)
		CFLAGS += -DDEBUG -g
		LDFLAGS := -g
else
		CFLAGS += -O2
endif



$(PROG): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<


ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean print_vars)))
-include $(DEPFILES)
endif

.PHONY: all
all: $(PROG)
	$(MAKE) $(PROG)

.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(PROG) $(DEPFILES)

.PHONY: print_vars
print_vars:
	@echo "===print_vars==="
	@echo "PROG     = $(PROG)"
	@echo "DEBUG    = $(DEBUG)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "SOURCES  = $(SOURCES)"
	@echo "OBJECTS  = $(OBJECTS)"
	@echo "DEPFILES = $(DEPFILES)"

