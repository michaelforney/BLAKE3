#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blake3_impl.h"

// Declarations for implementation-specific functions.
void blake3_compress_in_place_portable(uint32_t cv[8],
                                       const uint8_t block[BLAKE3_BLOCK_LEN],
                                       uint8_t block_len, uint64_t counter,
                                       uint8_t flags);

void blake3_compress_xof_portable(const uint32_t cv[8],
                                  const uint8_t block[BLAKE3_BLOCK_LEN],
                                  uint8_t block_len, uint64_t counter,
                                  uint8_t flags, uint8_t out[64]);

void blake3_hash_many_portable(const uint8_t *const *inputs, size_t num_inputs,
                               size_t blocks, const uint32_t key[8],
                               uint64_t counter, bool increment_counter,
                               uint8_t flags, uint8_t flags_start,
                               uint8_t flags_end, uint8_t *out);

#if defined(WITH_ASM) && defined(__x86_64__)
void blake3_cpuid(uint32_t out[4], uint32_t id, uint32_t sid);
uint64_t blake3_xgetbv(void);

void blake3_compress_in_place_sse2(uint32_t cv[8],
                                   const uint8_t block[BLAKE3_BLOCK_LEN],
                                   uint8_t block_len, uint64_t counter,
                                   uint8_t flags);
void blake3_compress_xof_sse2(const uint32_t cv[8],
                              const uint8_t block[BLAKE3_BLOCK_LEN],
                              uint8_t block_len, uint64_t counter,
                              uint8_t flags, uint8_t out[64]);
void blake3_hash_many_sse2(const uint8_t *const *inputs, size_t num_inputs,
                           size_t blocks, const uint32_t key[8],
                           uint64_t counter, bool increment_counter,
                           uint8_t flags, uint8_t flags_start,
                           uint8_t flags_end, uint8_t *out);

void blake3_compress_in_place_sse41(uint32_t cv[8],
                                    const uint8_t block[BLAKE3_BLOCK_LEN],
                                    uint8_t block_len, uint64_t counter,
                                    uint8_t flags);
void blake3_compress_xof_sse41(const uint32_t cv[8],
                               const uint8_t block[BLAKE3_BLOCK_LEN],
                               uint8_t block_len, uint64_t counter,
                               uint8_t flags, uint8_t out[64]);
void blake3_hash_many_sse41(const uint8_t *const *inputs, size_t num_inputs,
                            size_t blocks, const uint32_t key[8],
                            uint64_t counter, bool increment_counter,
                            uint8_t flags, uint8_t flags_start,
                            uint8_t flags_end, uint8_t *out);

void blake3_hash_many_avx2(const uint8_t *const *inputs, size_t num_inputs,
                           size_t blocks, const uint32_t key[8],
                           uint64_t counter, bool increment_counter,
                           uint8_t flags, uint8_t flags_start,
                           uint8_t flags_end, uint8_t *out);

void blake3_compress_in_place_avx512(uint32_t cv[8],
                                     const uint8_t block[BLAKE3_BLOCK_LEN],
                                     uint8_t block_len, uint64_t counter,
                                     uint8_t flags);
void blake3_compress_xof_avx512(const uint32_t cv[8],
                                const uint8_t block[BLAKE3_BLOCK_LEN],
                                uint8_t block_len, uint64_t counter,
                                uint8_t flags, uint8_t out[64]);
void blake3_hash_many_avx512(const uint8_t *const *inputs, size_t num_inputs,
                             size_t blocks, const uint32_t key[8],
                             uint64_t counter, bool increment_counter,
                             uint8_t flags, uint8_t flags_start,
                             uint8_t flags_end, uint8_t *out);

enum {
  SSE2   = 1 << 0,
  SSE41  = 1 << 1,
  AVX2   = 1 << 2,
  AVX512 = 1 << 3,
};

int blake3_cpu_features;

void blake3_detect_cpu_features(void) {
#if defined(__x86_64__)
  enum { EAX, EBX, ECX, EDX };
  uint32_t regs[4], xcr0;
  int features = 0;

  blake3_cpuid(regs, 1, 0);
  if (regs[EDX] & (1UL << 26))
    features |= SSE2;
  if (regs[ECX] & (1UL << 19))
    features |= SSE41;
  // OSXSAVE
  if (regs[ECX] & (1UL << 27)) {
    blake3_cpuid(regs, 0, 0);
    if (regs[EAX] >= 7) {
      blake3_cpuid(regs, 7, 0);
      xcr0 = blake3_xgetbv();
      // AVX2 and XCR0 SSE, AVX
      if (regs[EBX] & (1UL << 5) && (xcr0 & 0x06) == 0x06)
        features |= AVX2;
      // AVX512F, AVX512VL and XCR0 Opmask, ZMM_Hi256, Hi16_ZMM
      if (regs[EBX] & (1UL << 31 | 1UL << 16) && (xcr0 & 0xe0) == 0xe0)
        features |= AVX512;
    }
  }
  blake3_cpu_features = features;
#endif
}
#endif

void blake3_compress_in_place(uint32_t cv[8],
                              const uint8_t block[BLAKE3_BLOCK_LEN],
                              uint8_t block_len, uint64_t counter,
                              uint8_t flags) {
#if defined(WITH_ASM) && defined(__x86_64__)
  if (blake3_cpu_features & AVX512) {
    blake3_compress_in_place_avx512(cv, block, block_len, counter, flags);
    return;
  }
  if (blake3_cpu_features & SSE41) {
    blake3_compress_in_place_sse41(cv, block, block_len, counter, flags);
    return;
  }
  if (blake3_cpu_features & SSE2) {
    blake3_compress_in_place_sse2(cv, block, block_len, counter, flags);
    return;
  }
#endif
  blake3_compress_in_place_portable(cv, block, block_len, counter, flags);
}

void blake3_compress_xof(const uint32_t cv[8],
                         const uint8_t block[BLAKE3_BLOCK_LEN],
                         uint8_t block_len, uint64_t counter, uint8_t flags,
                         uint8_t out[64]) {
#if defined(WITH_ASM) && defined(__x86_64__)
  if (blake3_cpu_features & AVX512) {
    blake3_compress_xof_avx512(cv, block, block_len, counter, flags, out);
    return;
  }
  if (blake3_cpu_features & SSE41) {
    blake3_compress_xof_sse41(cv, block, block_len, counter, flags, out);
    return;
  }
  if (blake3_cpu_features & SSE2) {
    blake3_compress_xof_sse2(cv, block, block_len, counter, flags, out);
    return;
  }
#endif
  blake3_compress_xof_portable(cv, block, block_len, counter, flags, out);
}

void blake3_hash_many(const uint8_t *const *inputs, size_t num_inputs,
                      size_t blocks, const uint32_t key[8], uint64_t counter,
                      bool increment_counter, uint8_t flags,
                      uint8_t flags_start, uint8_t flags_end, uint8_t *out) {
#if defined(WITH_ASM) && defined(__x86_64__)
  if (blake3_cpu_features & AVX512) {
    blake3_hash_many_avx512(inputs, num_inputs, blocks, key, counter,
                            increment_counter, flags, flags_start, flags_end,
                            out);
    return;
  }
  if (blake3_cpu_features & AVX2) {
    blake3_hash_many_avx2(inputs, num_inputs, blocks, key, counter,
                          increment_counter, flags, flags_start, flags_end,
                          out);
    return;
  }
  if (blake3_cpu_features & SSE41) {
    blake3_hash_many_sse41(inputs, num_inputs, blocks, key, counter,
                           increment_counter, flags, flags_start, flags_end,
                           out);
    return;
  }
  if (blake3_cpu_features & SSE2) {
    blake3_hash_many_sse2(inputs, num_inputs, blocks, key, counter,
                          increment_counter, flags, flags_start, flags_end,
                          out);
    return;
  }
#endif

  blake3_hash_many_portable(inputs, num_inputs, blocks, key, counter,
                            increment_counter, flags, flags_start, flags_end,
                            out);
}

// The dynamically detected SIMD degree of the current platform.
size_t blake3_simd_degree(void) {
#if defined(WITH_ASM) && defined(__x86_64__)
  if (blake3_cpu_features & AVX512)
    return 16;
  if (blake3_cpu_features & AVX2)
    return 8;
  if (blake3_cpu_features & SSE41)
    return 4;
  if (blake3_cpu_features & SSE2)
    return 4;
#endif
  return 1;
}
