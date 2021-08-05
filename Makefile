ARCH ?= amd64
STATIC ?= 0
ARCHS32 := i386 arm
ARCHS64 := amd64 arm64

SRC = dlopen.c dlsym.c dlclose.c $(wildcard syscalls/*.c) $(wildcard libc/*.c)
HEADER = tinyld.h internals.h syscalls.h libc.h
OBJS = $(SRC:.c=.o)
LDFLAGS =
CFLAGS = -Wall -fPIC -nostdlib
# CFLAGS += -pipe -Wall -Wextra -fPIC -fno-ident -fno-stack-protector -U _FORTIFY_SOURCE
# CFLAGS += -DSTDLIB=$(STDLIB)
CFLAGS_i386 = -m32

TESTS = $(wildcard test/*.c)

ifeq "$(STATIC)" "1"
	CFLAGS += -static
endif

all: tinyld tinyld.a

tinyld: prebuild main.c $(OBJS)
	$(CC) -o $@ $(filter-out prebuild, $^) -O2 $(CFLAGS) $(LDFLAGS)

tinyld.a: prebuild $(OBJS)
	ar rcs $@ $(filter-out prebuild, $^)

%.o: %.c $(HEADER)
	$(CC) -c -o $@ $< $(LDFLAGS)

prebuild:
	rm arch/target
	ln -s $(ARCH) arch/target

test: tinyld $(TESTS:.c=.so)
	$(foreach var,$(TESTS:.c=.so),./tinyld $(var);)

test/%.so: test/%.c
	gcc -shared -fPIC -o $@ $< $(CFLAGS) -masm=intel $(LDFLAGS)

clean:
	rm -f tinyld tinyld.a $(OBJS) test/*.so
