#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

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

#if defined(__x86_64__)
void blake3_cpuid(uint32_t out[4], uint32_t id, uint32_t sid);
uint64_t blake3_xgetbv(void);

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
#endif

enum cpu_feature {
  SSE2 = 1 << 0,
  SSSE3 = 1 << 1,
  SSE41 = 1 << 2,
  AVX = 1 << 3,
  AVX2 = 1 << 4,
  AVX512F = 1 << 5,
  AVX512VL = 1 << 6,
  /* ... */
  UNDEFINED = 1 << 30
};

static enum cpu_feature g_cpu_features = UNDEFINED;
static pthread_once_t get_cpu_features_once = PTHREAD_ONCE_INIT;

static void get_cpu_features() {
#if defined(__x86_64__)
  enum { EAX, EBX, ECX, EDX };
  uint32_t regs[4] = {0};
  int max_id;

  g_cpu_features = 0;
  blake3_cpuid(regs, 0, 0);
  max_id = regs[EAX];
  blake3_cpuid(regs, 1, 0);
  if (regs[EDX] & (1UL << 26))
    g_cpu_features |= SSE2;
  if (regs[ECX] & (1UL << 0))
    g_cpu_features |= SSSE3;
  if (regs[ECX] & (1UL << 19))
    g_cpu_features |= SSE41;

  if (regs[ECX] & (1UL << 27)) { // OSXSAVE
    const uint64_t mask = blake3_xgetbv();
    if ((mask & 6) == 6) { // SSE and AVX states
      if (regs[ECX] & (1UL << 28))
        g_cpu_features |= AVX;
      if (max_id >= 7) {
        blake3_cpuid(regs, 7, 0);
        if (regs[EBX] & (1UL << 5))
          g_cpu_features |= AVX2;
        if ((mask & 224) == 224) { // Opmask, ZMM_Hi256, Hi16_Zmm
          if (regs[EBX] & (1UL << 31))
            g_cpu_features |= AVX512VL;
          if (regs[EBX] & (1UL << 16))
            g_cpu_features |= AVX512F;
        }
      }
    }
  }
#endif
}

void blake3_compress_in_place(uint32_t cv[8],
                              const uint8_t block[BLAKE3_BLOCK_LEN],
                              uint8_t block_len, uint64_t counter,
                              uint8_t flags) {
#if defined(__x86_64__)
  pthread_once(&get_cpu_features_once, get_cpu_features);
  if (g_cpu_features & AVX512VL) {
    blake3_compress_in_place_avx512(cv, block, block_len, counter, flags);
    return;
  }
  if (g_cpu_features & SSE41) {
    blake3_compress_in_place_sse41(cv, block, block_len, counter, flags);
    return;
  }
#endif
  blake3_compress_in_place_portable(cv, block, block_len, counter, flags);
}

void blake3_compress_xof(const uint32_t cv[8],
                         const uint8_t block[BLAKE3_BLOCK_LEN],
                         uint8_t block_len, uint64_t counter, uint8_t flags,
                         uint8_t out[64]) {
#if defined(__x86_64__)
  pthread_once(&get_cpu_features_once, get_cpu_features);
  if (g_cpu_features & AVX512VL) {
    blake3_compress_xof_avx512(cv, block, block_len, counter, flags, out);
    return;
  }
  if (g_cpu_features & SSE41) {
    blake3_compress_xof_sse41(cv, block, block_len, counter, flags, out);
    return;
  }
#endif
  blake3_compress_xof_portable(cv, block, block_len, counter, flags, out);
}

void blake3_hash_many(const uint8_t *const *inputs, size_t num_inputs,
                      size_t blocks, const uint32_t key[8], uint64_t counter,
                      bool increment_counter, uint8_t flags,
                      uint8_t flags_start, uint8_t flags_end, uint8_t *out) {
#if defined(__x86_64__)
  pthread_once(&get_cpu_features_once, get_cpu_features);
  if ((g_cpu_features & (AVX512F|AVX512VL)) == (AVX512F|AVX512VL)) {
    blake3_hash_many_avx512(inputs, num_inputs, blocks, key, counter,
                            increment_counter, flags, flags_start, flags_end,
                            out);
    return;
  }
  if (g_cpu_features & AVX2) {
    blake3_hash_many_avx2(inputs, num_inputs, blocks, key, counter,
                          increment_counter, flags, flags_start, flags_end,
                          out);
    return;
  }
  if (g_cpu_features & SSE41) {
    blake3_hash_many_sse41(inputs, num_inputs, blocks, key, counter,
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
#if defined(__x86_64__)
  pthread_once(&get_cpu_features_once, get_cpu_features);
  if ((g_cpu_features & (AVX512F|AVX512VL)) == (AVX512F|AVX512VL))
    return 16;
  if (g_cpu_features & AVX2)
    return 8;
  if (g_cpu_features & SSE41)
    return 4;
#endif
  return 1;
}
