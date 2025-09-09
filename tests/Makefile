CFLAGS=-Wall -Wextra -I../pine/include -ggdb
LDFLAGS=-L../pine/lib -lpine

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
else
	LDFLAGS+=-lopengl32 -lWs2_32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
endif

SOURCES=$(wildcard *.c)
TESTS=$(addprefix bin/,$(SOURCES:%.c=%))

all: $(TESTS)


$(TESTS):bin/%: ./%.c bin 
	@echo "  CC	$@"
	@$(CC) $< $(LDFLAGS) $(CFLAGS) -o $@

bin:
	@mkdir -p $@