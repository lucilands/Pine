CFLAGS=-Wall -Wextra -std=c99 -Iinclude -ggdb
include backend.mk

EXCLUDED_SOURCES=$(foreach b,$(filter-out $(BACKEND),$(BACKENDS)),%_$(b).c)
SOURCES=$(filter-out $(EXCLUDED_SOURCES),$(wildcard src/*.c))
OBJECTS=$(addprefix build/,$(notdir $(SOURCES:%.c=%.o)))

ifeq ($(BACKEND),wl)
	WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)
	WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
	PROTOCOLS=$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml
	PROTOCOL_DIR=build/protocols
	PROTOCOL_SOURCES=$(addprefix $(PROTOCOL_DIR)/,$(notdir $(PROTOCOLS:%.xml=%-client-protocol.c)))
	PROTOCOL_HEADERS=$(PROTOCOL_SOURCES:%.c=%.h)
	PROTOCOL_OBJECTS=$(PROTOCOL_SOURCES:%.c=%.o)
	CFLAGS+=-I$(PROTOCOL_DIR)
endif

TARGET=lib/libpine.a

all: $(TARGET)

$(TARGET): $(OBJECTS) $(PROTOCOL_OBJECTS) | lib
	@echo "  LD	$@"
	@$(AR) rcs $@ $(OBJECTS) $(PROTOCOL_OBJECTS)

$(OBJECTS):build/%.o: src/%.c | build
	@echo "  CC	$@"
	@$(CC) -c $< $(CFLAGS) -o $@

# Backend sources may include the generated protocol headers.
$(OBJECTS): $(PROTOCOL_HEADERS)

vpath %.xml $(dir $(PROTOCOLS))
# Keep the generated sources around instead of treating them as intermediates.
.SECONDARY: $(PROTOCOL_SOURCES)

$(PROTOCOL_DIR)/%-client-protocol.h: %.xml | $(PROTOCOL_DIR)
	@echo "  GEN	$@"
	@$(WAYLAND_SCANNER) client-header $< $@

$(PROTOCOL_DIR)/%-client-protocol.c: %.xml | $(PROTOCOL_DIR)
	@echo "  GEN	$@"
	@$(WAYLAND_SCANNER) private-code $< $@

$(PROTOCOL_DIR)/%.o: $(PROTOCOL_DIR)/%.c
	@echo "  CC	$@"
	@$(CC) -c $< $(CFLAGS) -o $@

build lib $(PROTOCOL_DIR):
	@mkdir -p $@

clean:
	rm -r build lib
