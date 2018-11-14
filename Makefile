TARGET=cspfuck
BUILDDIR=bin/
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-Werror -Wall -g -fPIC -DNDEBUG -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic -std=c11 -O2

all: main.c
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -o $(BUILDDIR)$(TARGET) $(CFLAGS)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

uninstall:
	rm -rf $(PREFIX)$(TARGET)
