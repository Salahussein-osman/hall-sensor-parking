CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
LDLIBS = -lm

SRC = src/debounce.c src/walls.c src/expr.c src/machine.c src/memory.c
HDR = $(wildcard src/*.h)

all: wallbreaker

wallbreaker: $(SRC) src/main.c $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SRC) src/main.c $(LDLIBS)

tests/run_tests: $(SRC) tests/test.c $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(SRC) tests/test.c $(LDLIBS)

test: tests/run_tests
	./tests/run_tests

bench: wallbreaker
	./wallbreaker bench

clean:
	rm -f wallbreaker tests/run_tests

.PHONY: all test bench clean
