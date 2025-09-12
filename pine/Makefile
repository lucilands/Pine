CFLAGS=-Wall -Wextra -std=c99 -Iinclude -ggdb
OSPOSTFIXES=_w32 _x11
ifeq ($(OS),Windows_NT)
	OSPOSTFIX=_w32
	OSNONPOSTFIX=_x11
else
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		OSPOSTFIX=_x11
		OSNONPOSTFIX=_w32
	endif
endif

SOURCES=$(filter-out %$(OSNONPOSTFIX).c, $(wildcard src/*.c))
OBJECTS=$(addprefix build/,$(notdir $(SOURCES:%.c=%.o)))

TARGET=lib/libpine.a

all: $(TARGET)

$(TARGET): $(OBJECTS) lib
	@echo "  LD	$@"
	@$(AR) rcs $@ $(OBJECTS)

$(OBJECTS):build/%.o: src/%.c build
	@echo "  CC	$@"
	@$(CC) -c $< $(CFLAGS) -o $@

build lib:
	@mkdir -p $@

clean:
	rm -r build lib
