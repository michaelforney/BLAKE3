.POSIX:

PREFIX?=/usr/local
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/lib
INCDIR?=$(PREFIX)/include
CFLAGS+=-Wall -Wpedantic -pthread
LDFLAGS+=-pthread
ARFLAGS=cr

BLAKE3_OBJ=\
	blake3.o\
	blake3_dispatch.o\
	blake3_portable.o\
	blake3_cpuid.o\
	blake3_avx2_x86-64_unix.o\
	blake3_avx512_x86-64_unix.o\
	blake3_sse41_x86-64_unix.o

.PHONY: all
all: b3sum libblake3.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.S.o:
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

libblake3.a: $(BLAKE3_OBJ)
	$(AR) $(ARFLAGS) $@ $(BLAKE3_OBJ)

b3sum: b3sum.o libblake3.a
	$(CC) $(LDFLAGS) -o $@ b3sum.o libblake3.a

.PHONY: install
install: b3sum libblake3.a
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCDIR)
	cp b3sum $(DESTDIR)$(BINDIR)
	cp libblake3.a $(DESTDIR)$(LIBDIR)
	cp blake3.h $(DESTDIR)$(INCDIR)

.PHONY: check
check: b3sum
	./test.py

.PHONY: clean
clean:
	rm -f b3sum b3sum.o libblake3.a $(BLAKE3_OBJ)
