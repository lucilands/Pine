MAKEFLAGS += --no-print-directory

CFLAGS=-Wall -Wextra -Ipine/include -ggdb
LDFLAGS=-Lpine/lib/ -lpine -ggdb

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU
else
	LDFLAGS+=-lopengl32 -lWs2_32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
endif

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
