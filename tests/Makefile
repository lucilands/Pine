CFLAGS=-Wall -Wextra -I../pine/include -ggdb
LDFLAGS=-L../pine/lib -lpine

include ../pine/backend.mk
ifeq ($(BACKEND),w32)
	LDFLAGS+=-lopengl32 -lWs2_32 -lole32 -lcomctl32 -lgdi32 -lcomdlg32 -luuid
else ifeq ($(BACKEND),wl)
	LDFLAGS+=-lwayland-client -lwayland-cursor -lwayland-egl -lEGL -lxkbcommon -lGL -lGLU
else
	LDFLAGS+=-lX11 -lGL -lGLU -lXrender
endif

SOURCES=$(wildcard *.c)
TESTS=$(addprefix bin/,$(SOURCES:%.c=%))

all: $(TESTS)


$(TESTS):bin/%: ./%.c bin 
	@echo "  CC	$@"
	@$(CC) $< $(LDFLAGS) $(CFLAGS) -o $@

bin:
	@mkdir -p $@