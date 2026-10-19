MAKEFLAGS += --no-print-directory

CFLAGS=-Wall -Wextra -Ipine/include -ggdb
LDFLAGS=-Lpine/lib/ -lpine -ggdb

include pine/backend.mk
ifeq ($(BACKEND),w32)
	LDFLAGS+=-lopengl32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
else ifeq ($(BACKEND),wl)
	LDFLAGS+=-lwayland-client -lwayland-egl -lEGL -lxkbcommon -lGL -lGLU
else
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
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
	@cd pine && make CC=$(CC) BACKEND=$(BACKEND)

build bin:
	@mkdir -p $@

tests: pine
	@cd ./tests && make CC=$(CC) BACKEND=$(BACKEND)

clean:
	rm -r build bin
	cd pine && make clean
