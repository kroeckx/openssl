/*
 * Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL licenses, (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://www.openssl.org/source/license.html
 * or in the file LICENSE in the source distribution.
 */

/*
 * Derived from the BLAKE2 reference implementation written by Samuel Neves.
 * More information about the BLAKE2 hash function and its implementations
 * can be found at https://blake2.net.
 */

#include <string.h>
#include "e_os.h"

static ossl_inline uint32_t load32(const void *src)
{
    const union {
        long one;
        char little;
    } is_endian = { 1 };

    if (is_endian.little) {
        uint32_t w;
        memcpy(&w, src, sizeof(w));
        return w;
    } else {
        const uint8_t *p = (const uint8_t *)src;
        uint32_t w = *p++;
        w |= (uint32_t)(*p++) <<  8;
        w |= (uint32_t)(*p++) << 16;
        w |= (uint32_t)(*p++) << 24;
        return w;
    }
}

static ossl_inline uint64_t load64(const void *src)
{
    const union {
        long one;
        char little;
    } is_endian = { 1 };

    if (is_endian.little) {
        uint64_t w;
        memcpy(&w, src, sizeof(w));
        return w;
    } else {
        const uint8_t *p = (const uint8_t *)src;
        uint64_t w = *p++;
        w |= (uint64_t)(*p++) <<  8;
        w |= (uint64_t)(*p++) << 16;
        w |= (uint64_t)(*p++) << 24;
        w |= (uint64_t)(*p++) << 32;
        w |= (uint64_t)(*p++) << 40;
        w |= (uint64_t)(*p++) << 48;
        w |= (uint64_t)(*p++) << 56;
        return w;
    }
}

static ossl_inline void store32(void *dst, uint32_t w)
{
    const union {
        long one;
        char little;
    } is_endian = { 1 };

    if (is_endian.little) {
        memcpy(dst, &w, sizeof(w));
    } else {
        uint8_t *p = (uint8_t *)dst;
        int i;

        for (i = 0; i < 4; i++)
            p[i] = (uint8_t)(w >> (8 * i));
    }
}

static ossl_inline void store64(void *dst, uint64_t w)
{
    const union {
        long one;
        char little;
    } is_endian = { 1 };

    if (is_endian.little) {
        memcpy(dst, &w, sizeof(w));
    } else {
        uint8_t *p = (uint8_t *)dst;
        int i;

        for (i = 0; i < 8; i++)
            p[i] = (uint8_t)(w >> (8 * i));
    }
}

static ossl_inline uint64_t load48(const void *src)
{
    const uint8_t *p = (const uint8_t *)src;
    uint64_t w = *p++;
    w |= (uint64_t)(*p++) <<  8;
    w |= (uint64_t)(*p++) << 16;
    w |= (uint64_t)(*p++) << 24;
    w |= (uint64_t)(*p++) << 32;
    w |= (uint64_t)(*p++) << 40;
    return w;
}

static ossl_inline void store48(void *dst, uint64_t w)
{
    uint8_t *p = (uint8_t *)dst;
    *p++ = (uint8_t)w;
    w >>= 8;
    *p++ = (uint8_t)w;
    w >>= 8;
    *p++ = (uint8_t)w;
    w >>= 8;
    *p++ = (uint8_t)w;
    w >>= 8;
    *p++ = (uint8_t)w;
    w >>= 8;
    *p++ = (uint8_t)w;
}

static ossl_inline uint32_t rotr32(const uint32_t w, const unsigned c)
{
    return (w >> c) | (w << (32 - c));
}

static ossl_inline uint64_t rotr64(const uint64_t w, const unsigned c)
{
    return (w >> c) | (w << (64 - c));
}
