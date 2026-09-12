CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_GNU_SOURCE -fPIC -Isrc
LDFLAGS =
TARGET = findmax
LIBRARY = libfindmax.so.1.0.0
LIBRARY_SONAME = libfindmax.so.1
LIBRARY_LINK = libfindmax.so
VPATH = src
SOURCES = main.c file_ops.c format.c heap.c
LIB_SOURCES = file_ops.c format.c logger.c
TEST_SOURCES = test_findmax.c file_ops.c format.c
OBJECTS = $(SOURCES:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Installation directories
PREFIX ?= /usr
DESTDIR ?=
BINDIR = $(PREFIX)/bin
MULTIARCH ?= $(shell dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || echo "x86_64-linux-gnu")
LIBDIR = $(PREFIX)/lib/$(MULTIARCH)
INCLUDEDIR = $(PREFIX)/include
MANDIR = $(PREFIX)/share/man/man1
COMPLETIONDIR = $(PREFIX)/share/bash-completion/completions

.PHONY: all clean install uninstall test optimized

all: $(TARGET) $(LIBRARY)

test_findmax: $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o test_findmax $(LDFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(LIBRARY): $(LIB_OBJECTS)
	$(CC) -shared -Wl,-soname,$(LIBRARY_SONAME) -o $(LIBRARY) $(LIB_OBJECTS) $(LDFLAGS)
	ln -sf $(LIBRARY) $(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(LIBRARY_LINK)

%.o: %.c src/findmax.h
	$(CC) $(CFLAGS) -c $< -o $@

test_findmax.o: tests/test_findmax.c src/findmax.h
	$(CC) $(CFLAGS) -c $< -o $@

optimized: SOURCES = heap.c file_ops.c format.c
optimized: CFLAGS += -DUSE_OPTIMIZED
optimized: $(TARGET)

clean:
	rm -f $(OBJECTS) $(LIB_OBJECTS) $(TEST_OBJECTS) $(TARGET) $(LIBRARY) $(LIBRARY_SONAME) $(LIBRARY_LINK) test_findmax

install: $(TARGET) $(LIBRARY) findmax-completion.bash
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(COMPLETIONDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 $(LIBRARY) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	install -m 644 src/findmax.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 findmax-completion.bash $(DESTDIR)$(COMPLETIONDIR)/findmax
	ldconfig -n $(DESTDIR)$(LIBDIR) 2>/dev/null || true

uninstall:
	sudo rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	sudo rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	sudo rm -f $(DESTDIR)$(INCLUDEDIR)/findmax.h
	sudo rm -f $(DESTDIR)$(COMPLETIONDIR)/findmax

test: test_findmax
	./test_findmax
