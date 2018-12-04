TARGET=cspfuck
BUILDDIR=bin/
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-std=c11 -O2 -g -Wno-gnu -lpthread
DEVFLAGS=-Werror -Wall -g -fPIC -DNDEBUG -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic -O0

all: main.c
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -o $(BUILDDIR)$(TARGET) $(CFLAGS)

no_wrap: main.c
	CFLAGS=-DNO_WRAP make

dev: main.c
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -o $(BUILDDIR)$(TARGET) $(CFLAGS) $(DEVFLAGS)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

uninstall:
	rm -rf $(PREFIX)$(TARGET)

clean:
	rm -rf $(BUILDDIR)
