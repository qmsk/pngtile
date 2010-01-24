# :set noexpandtab
CFLAGS_ALL = -Wall -std=gnu99
CFLAGS_DBG = -g
CFLAGS_REL = -O2

CFLAGS_SEL = ${CFLAGS_REL}

# warnings, and use C99 with GNU extensions
CFLAGS = ${CFLAGS_ALL} ${CFLAGS_SEL}

# preprocessor flags
CPPFLAGS = -Iinclude -Isrc/

# libraries to use
LOADLIBES = -lpng -lpthread

# output name
DIST_NAME = 78949E-as2
DIST_RESOURCES = README "Learning Diary.pdf" $(shell "echo python/*.{py,pyx}")

all: depend lib/libpngtile.so bin/util

lib/libpngtile.so : \
	build/obj/lib/ctx.o build/obj/lib/image.o build/obj/lib/cache.o build/obj/lib/tile.o build/obj/lib/png.o build/obj/lib/error.o \
	build/obj/shared/util.o build/obj/shared/log.o

lib/pypngtile.so : \
	lib/libpngtile.so

bin/util: \
	lib/libpngtile.so \
	build/obj/shared/log.o

SRC_PATHS = $(wildcard src/*/*.c)
SRC_NAMES = $(patsubst src/%,%,$(SRC_PATHS))
SRC_DIRS = $(dir $(SRC_NAMES))

.PHONY : dirs clean depend dist

dirs: 
	mkdir -p bin lib dist
	mkdir -p $(SRC_DIRS:%=build/deps/%)
	mkdir -p $(SRC_DIRS:%=build/obj/%)

clean:
	rm -f build/obj/*/*.o build/deps/*/*.d
	rm -f bin/* lib/*.so run/*
	rm -rf dist/*

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
bin/% : build/obj/%/main.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

# output libraries
lib/lib%.so :
	$(CC) -shared $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

build/pyx/%.c : src/py/%.pyx
	cython -o $@ $<

build/obj/py/%.o : build/pyx/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

lib/py%.so : build/obj/py/%.o
	$(CC) -shared $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

dist:
	mkdir -p dist/$(DIST_NAME)
	cp -rv Makefile $(DIST_RESOURCES) src/ include/ dist/$(DIST_NAME)/
	rm dist/$(DIST_NAME)/src/*/.*.sw[op]
	make -C dist/$(DIST_NAME) dirs
	tar -C dist -czvf dist/$(DIST_NAME).tar.gz $(DIST_NAME)
	@echo "*** Output at dist/$(DIST_NAME).tar.gz"

