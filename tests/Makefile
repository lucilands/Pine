CFLAGS=-Wall -Wextra -I../pine/include -Iglad/include -ggdb
LDFLAGS=-L../pine/lib -lpine

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
else
	LDFLAGS+=-lopengl32 -lWs2_32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
endif

SOURCES=$(wildcard *.c)
GLAD=glad/src/glad.c
GLAD_OBJ=glad/src/glad.o
TESTS=$(addprefix bin/,$(SOURCES:%.c=%))

all: $(TESTS)

$(GLAD_OBJ): $(GLAD)
	@$(CC) -c $< $(CFLAGS) -o $@

$(TESTS):bin/%: ./%.c $(GLAD_OBJ) bin 
	@echo "  CC	$@"
	@$(CC) $< $(LDFLAGS) $(GLAD_OBJ) $(CFLAGS) -o $@

bin:
	@mkdir -p $@