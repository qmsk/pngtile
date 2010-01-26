# :set noexpandtab
CFLAGS_ALL = -Wall -std=gnu99
LDFLAGS_ALL =
CFLAGS_DBG = -g
CFLAGS_REL = -O2
CFLAGS_PRF = -g -O2 -pg
LDFLAGS_PRF = -pg

CFLAGS_SEL = ${CFLAGS_REL}
LDFLAGS_SEL = ${LDFLAGS_REL}

# warnings, and use C99 with GNU extensions
CFLAGS = ${CFLAGS_ALL} ${CFLAGS_SEL}
LDFLAGS = ${LDFLAGS_ALL} ${LDFLAGS_SEL}

# preprocessor flags
CPPFLAGS = -Iinclude -Isrc/

# libraries to use
LOADLIBES = -lpng -lpthread

# output name
DIST_NAME = pngtile-${shell hg id -i}
DIST_RESOURCES = README python/ pngtile/ static/ bin/

all: depend lib/libpngtile.so bin/pngtile

lib/libpngtile.so : \
	build/obj/lib/ctx.o build/obj/lib/image.o build/obj/lib/cache.o build/obj/lib/tile.o build/obj/lib/png.o build/obj/lib/error.o \
	build/obj/shared/util.o build/obj/shared/log.o

lib/libpngtile.a : \
	build/obj/lib/ctx.o build/obj/lib/image.o build/obj/lib/cache.o build/obj/lib/tile.o build/obj/lib/png.o build/obj/lib/error.o \
	build/obj/shared/util.o build/obj/shared/log.o

lib/pypngtile.so : \
	lib/libpngtile.so

bin/pngtile : \
	build/obj/pngtile/main.o \
	lib/libpngtile.so build/obj/shared/log.o

bin/pngtile-static : \
	build/obj/pngtile/main.o \
	lib/libpngtile.a

SRC_PATHS = $(wildcard src/*/*.c)
SRC_NAMES = $(patsubst src/%,%,$(SRC_PATHS))
SRC_DIRS = $(dir $(SRC_NAMES))

.PHONY : dirs clean depend dist

dist-clean : clean dirs

dirs: 
	mkdir -p bin lib dist
	mkdir -p $(SRC_DIRS:%=build/deps/%)
	mkdir -p $(SRC_DIRS:%=build/obj/%) build/obj/python

clean:
	rm -f build/obj/*/*.o build/deps/*/*.d
	rm -f bin/pngtile bin/pngtile-static lib/libpngtile.{a,so} run/*
	rm -f pngtile/*.pyc 
	rm -f */.*.swp

# .h dependencies
depend: $(SRC_NAMES:%.c=build/deps/%.d)

build/deps/%.d : src/%.c
	@set -e; rm -f $@; \
	 $(CC) -MM -MT __ $(CPPFLAGS) $< > $@.$$$$; \
	 sed 's,__[ :]*,build/obj/$*.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

include $(wildcard build/deps/*/*.d)

# build (potential) library targets with specific cflags
# XXX: just build everything with -fPIC?
build/obj/shared/%.o : src/shared/%.c
	$(CC) -c -fPIC $(CPPFLAGS) $(CFLAGS) $< -o $@

# XXX: hax in -pthread
build/obj/lib/%.o : src/lib/%.c
	$(CC) -c -fPIC -pthread $(CPPFLAGS) $(CFLAGS) $< -o $@

# general binary objects
build/obj/%.o : src/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

# output binaries
bin/% :
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

# output libraries
lib/lib%.so :
	$(CC) -shared $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

lib/lib%.a :
	$(AR) rc $@ $+

python/%.c : python/%.pyx
	cython -o $@ $<

build/obj/python/%.o : python/%.c
	$(CC) -c -fPIC -I/usr/include/python2.5 $(CPPFLAGS) $(CFLAGS) $< -o $@

lib/py%.so : build/obj/python/py%.o
	$(CC) -shared $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

dist:
	rm -rf dist/$(DIST_NAME)
	mkdir -p dist/$(DIST_NAME)
	cp -rv Makefile $(DIST_RESOURCES) src/ include/  dist/$(DIST_NAME)/
	rm dist/$(DIST_NAME)/src/*/.*.sw[op]
	make -C dist/$(DIST_NAME) dist-clean
	tar -C dist -czvf dist/$(DIST_NAME).tar.gz $(DIST_NAME)
	@echo "*** Output at dist/$(DIST_NAME).tar.gz"

