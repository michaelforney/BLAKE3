# b3sum

A C implementation of the `b3sum` tool, as well as a reusable library
`libblake3.a`. Based on the official [C implementation of BLAKE3].

[C implementation of BLAKE3]: https://github.com/BLAKE3-team/BLAKE3/tree/master/c

## Example

An example program that hashes bytes from standard input and prints the
result:

```c
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
  // Initialize the hasher.
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);

  // Read input bytes from stdin.
  unsigned char buf[65536];
  while (1) {
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0) {
      blake3_hasher_update(&hasher, buf, n);
    } else if (n == 0) {
      break; // end of file
    } else {
      fprintf(stderr, "read failed: %s\n", strerror(errno));
      exit(1);
    }
  }

  // Finalize the hash. BLAKE3_OUT_LEN is the default output length, 32 bytes.
  uint8_t output[BLAKE3_OUT_LEN];
  blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);

  // Print the hash as hexadecimal.
  for (size_t i = 0; i < BLAKE3_OUT_LEN; i++) {
    printf("%02x", output[i]);
  }
  printf("\n");
  return 0;
}
```

## API

### The Struct

```c
typedef struct {
  // private fields
} blake3_hasher;
```

An incremental BLAKE3 hashing state, which can accept any number of
updates. This implementation doesn't allocate any heap memory, but
`sizeof(blake3_hasher)` itself is relatively large, currently 1912 bytes
on x86-64. This size can be reduced by restricting the maximum input
length, as described in Section 5.4 of [the BLAKE3
spec](https://github.com/BLAKE3-team/BLAKE3-specs/blob/master/blake3.pdf),
but this implementation doesn't currently support that strategy.

### Common API Functions

```c
void blake3_hasher_init(
  blake3_hasher *self);
```

Initialize a `blake3_hasher` in the default hashing mode.

---

```c
void blake3_hasher_update(
  blake3_hasher *self,
  const void *input,
  size_t input_len);
```

Add input to the hasher. This can be called any number of times.

---

```c
void blake3_hasher_finalize(
  const blake3_hasher *self,
  uint8_t *out,
  size_t out_len);
```

Finalize the hasher and return an output of any length, given in bytes.
This doesn't modify the hasher itself, and it's possible to finalize
again after adding more input. The constant `BLAKE3_OUT_LEN` provides
the default output length, 32 bytes, which is recommended for most
callers.

Outputs shorter than the default length of 32 bytes (256 bits) provide
less security. An N-bit BLAKE3 output is intended to provide N bits of
first and second preimage resistance and N/2 bits of collision
resistance, for any N up to 256. Longer outputs don't provide any
additional security.

Shorter BLAKE3 outputs are prefixes of longer ones. Explicitly
requesting a short output is equivalent to truncating the default-length
output. (Note that this is different between BLAKE2 and BLAKE3.)

### Less Common API Functions

```c
void blake3_hasher_init_keyed(
  blake3_hasher *self,
  const uint8_t key[BLAKE3_KEY_LEN]);
```

Initialize a `blake3_hasher` in the keyed hashing mode. The key must be
exactly 32 bytes.

---

```c
void blake3_hasher_init_derive_key(
  blake3_hasher *self,
  const char *context);
```

Initialize a `blake3_hasher` in the key derivation mode. The context
string is given as an initialization parameter, and afterwards input key
material should be given with `blake3_hasher_update`. The context string
is a null-terminated C string which should be **hardcoded, globally
unique, and application-specific**. The context string should not
include any dynamic input like salts, nonces, or identifiers read from a
database at runtime. A good default format for the context string is
`"[application] [commit timestamp] [purpose]"`, e.g., `"example.com
2019-12-25 16:18:03 session tokens v1"`.

This function is intended for application code written in C. For
language bindings, see `blake3_hasher_init_derive_key_raw` below.

---

```c
void blake3_hasher_init_derive_key_raw(
  blake3_hasher *self,
  const void *context,
  size_t context_len);
```

As `blake3_hasher_init_derive_key` above, except that the context string
is given as a pointer to an array of arbitrary bytes with a provided
length. This is intended for writing language bindings, where C string
conversion would add unnecessary overhead and new error cases. Unicode
strings should be encoded as UTF-8.

Application code in C should prefer `blake3_hasher_init_derive_key`,
which takes the context as a C string. If you need to use arbitrary
bytes as a context string in application code, consider whether you're
violating the requirement that context strings should be hardcoded.

---

```c
void blake3_hasher_finalize_seek(
  const blake3_hasher *self,
  uint64_t seek,
  uint8_t *out,
  size_t out_len);
```

The same as `blake3_hasher_finalize`, but with an additional `seek`
parameter for the starting byte position in the output stream. To
efficiently stream a large output without allocating memory, call this
function in a loop, incrementing `seek` by the output length each time.

---

```c
void blake3_hasher_reset(
  blake3_hasher *self);
```

Reset the hasher to its initial state, prior to any calls to
`blake3_hasher_update`. Currently this is no different from calling
`blake3_hasher_init` or similar again. However, if this implementation gains
multithreading support in the future, and if `blake3_hasher` holds (optional)
threading resources, this function will reuse those resources. Until then, this
is mainly for feature compatibility with the Rust implementation.
