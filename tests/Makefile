CFLAGS=-Wall -Wextra -I../pine/include -ggdb
LDFLAGS=-L../pine/lib -lpine

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
	CFLAGS+=-Iglad_nix/include
	GLADDIR=glad_nix
else
	LDFLAGS+=-lopengl32 -lWs2_32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
	CFLAGS+=-Iglad_w32/include
	GLADDIR=glad_w32
endif

SOURCES=$(wildcard *.c)
GLAD=$(GLADDIR)/src/glad.c
GLAD_OBJ=$(GLADDIR)/src/glad.o
TESTS=$(addprefix bin/,$(SOURCES:%.c=%))

all: $(TESTS)

$(GLAD_OBJ): $(GLAD)
	@$(CC) -c $< $(CFLAGS) -o $@

$(TESTS):bin/%: ./%.c $(GLAD_OBJ) bin 
	@echo "  CC	$@"
	@$(CC) $< $(LDFLAGS) $(GLAD_OBJ) $(CFLAGS) -o $@

bin:
	@mkdir -p $@