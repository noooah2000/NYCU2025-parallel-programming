#ifndef PRNG_H
#define PRNG_H
#include <cstdint>
#include <immintrin.h> 

static inline uint32_t xs32(uint32_t &s) {
    s ^= (s << 13);
    s ^= (s >> 17);
    s ^= (s << 5);
    return s;
}

static inline float u31_to_neg1_pos1(uint32_t r) {
    float u01 = (r & 0x7fffffffu) * (1.0f / 2147483648.0f);
    return 2.0f * u01 - 1.0f;
}

static inline __m256i xs32_vec(__m256i s) {
    s = _mm256_xor_si256(s, _mm256_slli_epi32(s, 13));
    s = _mm256_xor_si256(s, _mm256_srli_epi32(s, 17));
    s = _mm256_xor_si256(s, _mm256_slli_epi32(s, 5));
    return s;
}

static inline __m256 u31_to_neg1_pos1_vec(__m256i r) {
    const __m256i mask31 = _mm256_set1_epi32(0x7fffffff);
    const __m256 inv_2p31 = _mm256_set1_ps(1.0f / 2147483648.0f);
    __m256i r31 = _mm256_and_si256(r, mask31);
    __m256 fr = _mm256_cvtepi32_ps(r31);
    __m256 u01 = _mm256_mul_ps(fr, inv_2p31);
    const __m256 two = _mm256_set1_ps(2.0f);
    const __m256 one = _mm256_set1_ps(1.0f);
    return _mm256_sub_ps(_mm256_mul_ps(two, u01), one);
}

static inline __m256i make_seed_vec(uint32_t base) {
    alignas(32) uint32_t s[8];
    for (int i = 0; i < 8; ++i) {
        uint32_t v = base
                   ^ (0x9E3779B9u * (i + 1))
                   ^ (0x85EBCA6Bu + 0x27d4eb2du * i);
        if (v == 0) v = 1u;
        s[i] = v;
    }
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(s));
}

#endif