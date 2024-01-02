
sources=main.c
target=visualiser


INCLUDES=-Ilibs/raylib/src
LINK=-Llibs/raylib/src -lraylib -lm

CFLAGS = -std=c99 $(LINK) $(INCLUDES)  -O3
CC:=clang

ifeq ($(OS),Windows_NT)
	RAYLIB_PLATFORM:=PLATFORM_DESKTOP
	target:=$(target).exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		RAYLIB_PLATFORM:=PLATFORM_DESKTOP
	else ifeq ($(UNAME_S),Darwin)
		RAYLIB_PLATFORM:=PLATFORM_DESKTOP
	else
		RAYLIB_PLATFORM:=PLATFORM_DESKTOP
	endif
endif


.PHONE: all raylib clean

all: raylib $(target)
	@echo Build complete

$(target): $(sources)
	$(CC) -o $(target) $(sources) $(CFLAGS)


raylib: libs/raylib/src/libraylib.a
	@echo Raylib build complete

libs/raylib/src/libraylib.a:
	@echo Building raylib
	@$(MAKE) -C libs/raylib/src CC=$(CC) PLATFORM=$(RAYLIB_PLATFORM)

clean:
	@if [ -f $(target) ]; then rm $(target); fi
	$(MAKE) -C libs/raylib/src clean
