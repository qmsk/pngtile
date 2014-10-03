# debug
CFLAGS_DBG = -g
LDFLAGS_DBG = 

# profile
CFLAGS_PRF = -g -O2 -pg
LDFLAGS_PRF = -pg

# release
CFLAGS_REL = -O2
LDFLAGS_REL =

# preprocessor flags
CPPFLAGS = -Iinclude -Isrc/
CFLAGS = -Wall -std=gnu99 -fPIC -pthread ${CFLAGS_REL}
LDFLAGS = ${LDFLAGS_ALL} ${LDFLAGS_REL}
LDLIBS = -lpng -lpthread

all: build lib bin lib/libpngtile.so bin/pngtile

# binary deps
lib/libpngtile.so : \
	build/lib/ctx.o build/lib/image.o build/lib/cache.o build/lib/tile.o build/lib/png.o build/lib/error.o \
	build/shared/util.o build/shared/log.o

lib/libpngtile.a : \
	build/lib/ctx.o build/lib/image.o build/lib/cache.o build/lib/tile.o build/lib/png.o build/lib/error.o \
	build/shared/util.o build/shared/log.o

bin/pngtile : \
	build/pngtile/main.o \
	lib/libpngtile.so build/shared/log.o

bin/pngtile-static : \
	build/pngtile/main.o \
	lib/libpngtile.a

SRC_PATHS = $(wildcard src/*/*.c)
SRC_DIRS = $(dir $(SRC_PATHS))

build:
	mkdir -p $(SRC_DIRS:src/%=build/%)

lib: 
	mkdir -p lib 

bin: 
	mkdir -p bin

# build obj files from src, with header deps
build/%.o: src/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) src/$*.c -o build/$*.o
	$(CC) -MM $(CPPFLAGS) src/$*.c -MT build/$*.o -MF build/$*.d

-include $(wildcard build/*/*.d)

# output libraries
lib/lib%.so:
	$(CC) -shared $(LDFLAGS) $+ $(LDLIBS) -o $@

lib/lib%.a:
	$(AR) rc $@ $+

# output binaries
bin/%:
	$(CC) $(LDFLAGS) $+ -o $@ $(LDLIBS)

clean:
	rm -f build/*/*.o build/*/*.d
	rm -f bin/pngtile bin/pngtile-static lib/*.so lib/*.a

# dist builds
DIST_NAME = pngtile-${shell hg id -i}
DIST_DEPS = 
DIST_RESOURCES = README pngtile/ static/ bin/

dist-clean: clean dirs

dist: dist-clean $(DIST_DEPS)
	mkdir -p dist
	rm -rf dist/$(DIST_NAME)
	mkdir -p dist/$(DIST_NAME)
	cp -rv Makefile $(DIST_RESOURCES) src/ include/  dist/$(DIST_NAME)/
	make -C dist/$(DIST_NAME) dist-clean
	tar -C dist -czvf dist/$(DIST_NAME).tar.gz $(DIST_NAME)
	@echo "*** Output at dist/$(DIST_NAME).tar.gz"


.PHONY : dirs clean depend dist-clean dist
