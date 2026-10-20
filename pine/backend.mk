# Shared backend selection, included by every Makefile in the tree.
# BACKEND is one of $(BACKENDS). Override with `make BACKEND=x11`.
BACKENDS=w32 x11 wl
ifeq ($(OS),Windows_NT)
	BACKEND?=w32
else
	BACKEND?=wl
endif

ifeq ($(filter $(BACKEND),$(BACKENDS)),)
$(error Unknown BACKEND '$(BACKEND)', expected one of: $(BACKENDS))
endif
