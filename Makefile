MAKEFLAGS += --no-print-directory

CFLAGS=-Wall -Wextra -Ipine/include -ggdb
LDFLAGS=-Lpine/lib/ -lpine -ggdb

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
else
	LDFLAGS+=-lopengl32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
endif

SOURCES=$(wildcard src/*.c)
OBJECTS=$(addprefix build/,$(notdir $(SOURCES:%.c=%.o)))
BUILD_TESTS=Yes

TARGET=bin/demo

.PHONY: all pine tests clean

ifeq ($(BUILD_TESTS),Yes)
all: $(TARGET) tests
else
all: $(TARGET)
endif

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

tests: pine
	@cd ./tests && make

clean:
