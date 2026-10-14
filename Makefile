CC ?= gcc
PREFIX ?= /usr/local
VERSION := $(shell tr -d '\n' < VERSION)
CPPFLAGS ?= -Iincludes
CPPFLAGS += -DPROJECT_VERSION=\"$(VERSION)\"
CFLAGS ?= -O2 -g
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic
LDLIBS := -lGL -lGLU -lglut -lm

TARGET := bin/curvas-parametricas-2d
SOURCES := $(shell find src -name '*.c' -print | sort)
OBJECTS := $(patsubst src/%.c,obj/%.o,$(SOURCES))

.PHONY: all run test benchmark clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(OBJECTS) $(LDLIBS) -o $@

obj/%.o: src/%.c
	@mkdir -p $(@D)
	case "$<" in src/opengl.c|src/curvas/*.c) extra=-finstrument-functions ;; src/performance.c) extra=-Wno-pedantic ;; *) extra= ;; esac; \
	$(CC) $(CPPFLAGS) $(CFLAGS) $$extra -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test:
	./tests/run_tests.sh

benchmark: $(TARGET)
	./$(TARGET) --benchmark 1000

clean:
	rm -rf obj bin

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/curvas-parametricas-2d

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/curvas-parametricas-2d
