PREFIX = opt

# debug
CFLAGS_DEV = -g
LDFLAGS_DEV = -Wl,-Rlib

# profile
CFLAGS_PRF = -g -O2 -pg
LDFLAGS_PRF = -pg

# release
CFLAGS_REL = -O2
LDFLAGS_REL = -Wl,-R${PREFIX}/lib

# preprocessor flags
CPPFLAGS = -Iinclude -Isrc
CFLAGS = -Wall -std=gnu99 -fPIC ${CFLAGS_DEV}
LDFLAGS = -Llib ${LDFLAGS_DEV}
LDLIBS_LIB = -lpng
LDLIBS_BIN = -lpngtile

DIRS = build lib bin
all: $(DIRS) lib/libpngtile.so lib/libpngtile.a bin/pngtile

LIB_DEPS = \
	build/lib/image.o \
	build/lib/cache.o \
	build/lib/tile.o \
	build/lib/png.o \
	build/lib/error.o \
	build/lib/log.o \
	build/lib/path.o

# binary deps
lib/libpngtile.so: ${LIB_DEPS}
lib/libpngtile.a: ${LIB_DEPS}

bin/pngtile: \
	build/pngtile/main.o \
	build/pngtile/log.o

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
	$(CC) -shared $(LDFLAGS) $+ $(LDLIBS_LIB) -o $@

lib/lib%.a:
	$(AR) rc $@ $+

# output binaries
bin/%:
	$(CC) $(LDFLAGS) $+ -o $@ $(LDLIBS_BIN)

clean:
	rm -f build/*/*.o build/*/*.d
	rm -f bin/pngtile bin/pngtile-static lib/*.so lib/*.a

# install
INSTALL_INCLUDE = include/pngtile.h
INSTALL_LIB = lib/libpngtile.so
INSTALL_BIN = bin/pngtile

install: $(DIRS) $(INSTALL_INCLUDE) $(INSTALL_LIB) $(INSTALL_BIN)
	install -d $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/include
	install -t $(PREFIX)/include $(INSTALL_INCLUDE)
	install -t $(PREFIX)/lib $(INSTALL_LIB)
	install -t $(PREFIX)/bin $(INSTALL_BIN)

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
