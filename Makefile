CC ?= gcc
PREFIX ?= /usr/local
VERSION := $(shell tr -d '\n' < VERSION)
CPPFLAGS ?= -Iincludes
CPPFLAGS += -DPROJECT_VERSION=\"$(VERSION)\"
CFLAGS ?= -O2 -g
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic -MMD -MP
LDLIBS := -lGL -lGLU -lglut -lm

TARGET := bin/curvas-parametricas-2d
SOURCES := $(shell find src -name '*.c' -print | sort)
OBJECTS := $(patsubst src/%.c,obj/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)

.PHONY: all run test benchmark format clean install uninstall

all: $(TARGET)

-include $(DEPENDS)

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

format:
	for source in $$(find src tests -type f -name '*.c' -print); do \
		clang-tidy -fix -checks='-*,readability-braces-around-statements' $$source -- \
			-std=c99 -Iincludes >/dev/null 2>&1 || exit 1; \
	done
	files="$$(find src includes tests -type f \( -name '*.c' -o -name '*.h' \) -print)"; \
	clang-format -i $$files; \
	perl -pi -e 's/\b(if|for|while|switch)\s*\(/$$1(/g; s/\) \{/)\{/g' $$files

clean:
	rm -rf obj bin

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/curvas-parametricas-2d

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/curvas-parametricas-2d
