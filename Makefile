ARCH ?= amd64
STATIC ?= 0
ARCHS32 := i386 arm
ARCHS64 := amd64 arm64

SRC = dlopen.c dlsym.c dlclose.c $(wildcard syscalls/*.c)
HEADER = tinyld.h types.h syscalls.h
OBJS = $(SRC:.c=.o)
LDFLAGS =
CFLAGS = -Wall
# CFLAGS += -pipe -Wall -Wextra -fPIC -fno-ident -fno-stack-protector -U _FORTIFY_SOURCE
# CFLAGS += -DSTDLIB=$(STDLIB)
CFLAGS_i386 = -m32

ifeq "$(filter $(ARCH),$(ARCHS32))" "$(ARCH)"
  	CFLAGS += -DELFCLASS=ELFCLASS32
else
  	CFLAGS += -DELFCLASS=ELFCLASS64
endif

ifeq "$(STATIC)" "1"
	CFLAGS += -static
endif

all: tinyld tinyld.a

tinyld: main.c $(OBJS)
	$(CC) -o $@ $^ -O2 $(CFLAGS) $(LDFLAGS)

tinyld.a: $(OBJS)
	ar rcs $@ $^

%.o: %.c $(HEADER)
	$(CC) -c -o $@ $< $(LDFLAGS)

clean:
	rm -f tinyld tinyld.a $(OBJS)
