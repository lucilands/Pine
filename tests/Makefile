CFLAGS=-Wall -Wextra -I../pine/include
LDFLAGS=-L../pine/lib -lpine

ifneq ($(OS),Windows_NT)
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
else
	LDFLAGS+=-lopengl32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
endif

SOURCES=$(wildcard *.c)
TESTS=$(addprefix bin/,$(SOURCES:%.c=%))


$(TESTS):bin/%: ./%.c bin 
	@echo "  CC	$@"
	@$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

bin:
	@mkdir $@