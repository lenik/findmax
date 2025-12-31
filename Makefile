CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_GNU_SOURCE -fPIC
LDFLAGS = 
TARGET = findmax
LIBRARY = libfindmax.so.1.0.0
LIBRARY_SONAME = libfindmax.so.1
LIBRARY_LINK = libfindmax.so
SOURCES = main.c file_ops.c format.c
LIB_SOURCES = libfindmax.c file_ops.c format.c
OBJECTS = $(SOURCES:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)

# Installation directories
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
MANDIR = $(PREFIX)/share/man/man1
COMPLETIONDIR = $(PREFIX)/share/bash-completion/completions

# Default target
all: $(TARGET) $(LIBRARY)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build shared library
$(LIBRARY): $(LIB_OBJECTS)
	$(CC) -shared -Wl,-soname,$(LIBRARY_SONAME) -o $(LIBRARY) $(LIB_OBJECTS) $(LDFLAGS)
	ln -sf $(LIBRARY) $(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(LIBRARY_LINK)

# Build object files
%.o: %.c findmax.h libfindmax.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build optimized version with heap
optimized: SOURCES = heap.c file_ops.c format.c
optimized: CFLAGS += -DUSE_OPTIMIZED
optimized: $(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(LIB_OBJECTS) $(TARGET) $(LIBRARY) $(LIBRARY_SONAME) $(LIBRARY_LINK)

# Install to system with DESTDIR and PREFIX support
install: $(TARGET) $(LIBRARY) findmax.1 findmax-completion.bash
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(MANDIR)
	install -d $(DESTDIR)$(COMPLETIONDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 $(LIBRARY) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	ln -sf $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	install -m 644 findmax.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 libfindmax.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 findmax.1 $(DESTDIR)$(MANDIR)/
	install -m 644 findmax-completion.bash $(DESTDIR)$(COMPLETIONDIR)/findmax
	ldconfig -n $(DESTDIR)$(LIBDIR) 2>/dev/null || true

# Install symlinks to development directory (hardcoded /usr)
install-symlinks:
	install -d /usr/bin
	install -d /usr/lib
	install -d /usr/include
	install -d /usr/share/man/man1
	install -d /usr/share/bash-completion/completions
	ln -sf $(PWD)/$(TARGET) /usr/bin/$(TARGET)
	ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(LIBRARY)
	ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(LIBRARY_SONAME)
	ln -sf $(PWD)/$(LIBRARY) /usr/lib/$(LIBRARY_LINK)
	ln -sf $(PWD)/findmax.h /usr/include/findmax.h
	ln -sf $(PWD)/libfindmax.h /usr/include/libfindmax.h
	ln -sf $(PWD)/findmax.1 /usr/share/man/man1/findmax.1
	ln -sf $(PWD)/findmax-completion.bash /usr/share/bash-completion/completions/findmax
	ldconfig 2>/dev/null || true

# Uninstall from system
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBRARY_LINK)
	rm -f $(DESTDIR)$(INCLUDEDIR)/findmax.h
	rm -f $(DESTDIR)$(INCLUDEDIR)/libfindmax.h
	rm -f $(DESTDIR)$(MANDIR)/findmax.1
	rm -f $(DESTDIR)$(COMPLETIONDIR)/findmax
	ldconfig -n $(DESTDIR)$(LIBDIR) 2>/dev/null || true

# Run tests
test: $(TARGET)
	@echo "Running basic tests..."
	@mkdir -p test_dir/subdir
	@touch test_dir/file1.txt test_dir/file2.txt test_dir/subdir/file3.txt
	@echo "Test 1: Find latest file in current directory"
	./$(TARGET) -t test_dir/
	@echo "Test 2: Find latest file recursively"
	./$(TARGET) -t -R test_dir/
	@echo "Test 3: Find largest file"
	./$(TARGET) -S test_dir/
	@echo "Test 4: Custom format"
	./$(TARGET) -t -F "%n %y" test_dir/
	@echo "Test 5: Find top 3 files"
	./$(TARGET) -t -R -3 test_dir/
	@rm -rf test_dir

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean install uninstall test debug optimized
