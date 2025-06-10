CFLAGS=-Wall -Wextra -Ipine/include -ggdb
LDFLAGS=-Lpine/lib/ -lpine -ggdb


SOURCES=$(wildcard src/*.c)
OBJECTS=$(addprefix build/,$(notdir $(SOURCES:%.c=%.o)))

TARGET=bin/demo

.PHONY: all pine

all: $(TARGET)

$(TARGET): $(OBJECTS) pine bin
	@echo "  LD	$@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJECTS):build/%.o: src/%.c build
	@echo "  CC	$@"
	@$(CC) -c $< $(CFLAGS) -o $@

pine:
	@cd pine && make CC=$(CC)

build bin:
	@mkdir -p $@