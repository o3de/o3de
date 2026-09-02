// Copyright (c) 2011 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// CityHash, by Geoff Pike and Jyrki Alakuijala
//
// This file provides CityHash64() and related functions.
//
// It's probably possible to create even faster hash functions by
// writing a program that systematically explores some of the space of
// possible hash functions, by using SIMD instructions, or by
// compromising on hash quality.

#include <city.h>

#include <algorithm>
#include <bit>
#include <cstring>

static_assert(
    std::endian::native == std::endian::little || std::endian::native == std::endian::big,
    "CityHash does not support mixed-endian targets.");

static constexpr std::uint32_t ByteSwap32(std::uint32_t value) noexcept
{
    return (value >> 24) | ((value >> 8) & 0x0000ff00U) | ((value << 8) & 0x00ff0000U) | (value << 24);
}

static constexpr std::uint64_t ByteSwap64(std::uint64_t value) noexcept
{
    return (static_cast<std::uint64_t>(ByteSwap32(static_cast<std::uint32_t>(value))) << 32) |
        ByteSwap32(static_cast<std::uint32_t>(value >> 32));
}

static constexpr std::uint32_t Uint32InExpectedOrder(std::uint32_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
    {
        return ByteSwap32(value);
    }
    return value;
}

static constexpr std::uint64_t Uint64InExpectedOrder(std::uint64_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
    {
        return ByteSwap64(value);
    }
    return value;
}

static std::uint64_t UNALIGNED_LOAD64(const char* p)
{
    std::uint64_t result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

static std::uint32_t UNALIGNED_LOAD32(const char* p)
{
    std::uint32_t result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

static std::uint64_t Fetch64(const char* p)
{
    return Uint64InExpectedOrder(UNALIGNED_LOAD64(p));
}

static std::uint32_t Fetch32(const char* p)
{
    return Uint32InExpectedOrder(UNALIGNED_LOAD32(p));
}

// Some primes between 2^63 and 2^64 for various uses.
constexpr std::uint64_t k0 = 0xc3a5c85c97cb3127ULL;
constexpr std::uint64_t k1 = 0xb492b66fbe98f273ULL;
constexpr std::uint64_t k2 = 0x9ae16a3b2f90404fULL;

// Magic numbers for 32-bit hashing. Copied from Murmur3.
constexpr std::uint32_t c1 = 0xcc9e2d51;
constexpr std::uint32_t c2 = 0x1b873593;

// A 32-bit to 32-bit integer hash copied from Murmur3.
static constexpr std::uint32_t fmix(std::uint32_t h) noexcept
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

static constexpr void Permute3(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c) noexcept
{
    const std::uint32_t originalA = a;
    a = c;
    c = b;
    b = originalA;
}

static constexpr std::uint32_t Mur(std::uint32_t a, std::uint32_t h) noexcept
{
    // Helper from Murmur3 for combining two 32-bit values.
    a *= c1;
    a = std::rotr(a, 17);
    a *= c2;
    h ^= a;
    h = std::rotr(h, 19);
    return h * 5 + 0xe6546b64;
}

static std::uint32_t Hash32Len13to24(const char* s, std::size_t len)
{
    std::uint32_t a = Fetch32(s - 4 + (len >> 1));
    std::uint32_t b = Fetch32(s + 4);
    std::uint32_t c = Fetch32(s + len - 8);
    std::uint32_t d = Fetch32(s + (len >> 1));
    std::uint32_t e = Fetch32(s);
    std::uint32_t f = Fetch32(s + len - 4);
    std::uint32_t h = static_cast<std::uint32_t>(len);

    return fmix(Mur(f, Mur(e, Mur(d, Mur(c, Mur(b, Mur(a, h)))))));
}

static std::uint32_t Hash32Len0to4(const char* s, std::size_t len)
{
    std::uint32_t b = 0;
    std::uint32_t c = 9;
    for (std::size_t i = 0; i < len; i++)
    {
        signed char v = static_cast<signed char>(s[i]);
        b = b * c1 + static_cast<std::uint32_t>(v);
        c ^= b;
    }
    return fmix(Mur(b, Mur(static_cast<std::uint32_t>(len), c)));
}

static std::uint32_t Hash32Len5to12(const char* s, std::size_t len)
{
    std::uint32_t a = static_cast<std::uint32_t>(len), b = a * 5, c = 9, d = b;
    a += Fetch32(s);
    b += Fetch32(s + len - 4);
    c += Fetch32(s + ((len >> 1) & 4));
    return fmix(Mur(c, Mur(b, Mur(a, d))));
}

std::uint32_t CityHash32(const char* s, std::size_t len)
{
    if (len <= 24)
    {
        if (len <= 4)
        {
            return Hash32Len0to4(s, len);
        }
        if (len <= 12)
        {
            return Hash32Len5to12(s, len);
        }
        return Hash32Len13to24(s, len);
    }

    // len > 24
    std::uint32_t h = static_cast<std::uint32_t>(len), g = c1 * h, f = g;
    std::uint32_t a0 = std::rotr(Fetch32(s + len - 4) * c1, 17) * c2;
    std::uint32_t a1 = std::rotr(Fetch32(s + len - 8) * c1, 17) * c2;
    std::uint32_t a2 = std::rotr(Fetch32(s + len - 16) * c1, 17) * c2;
    std::uint32_t a3 = std::rotr(Fetch32(s + len - 12) * c1, 17) * c2;
    std::uint32_t a4 = std::rotr(Fetch32(s + len - 20) * c1, 17) * c2;
    h ^= a0;
    h = std::rotr(h, 19);
    h = h * 5 + 0xe6546b64;
    h ^= a2;
    h = std::rotr(h, 19);
    h = h * 5 + 0xe6546b64;
    g ^= a1;
    g = std::rotr(g, 19);
    g = g * 5 + 0xe6546b64;
    g ^= a3;
    g = std::rotr(g, 19);
    g = g * 5 + 0xe6546b64;
    f += a4;
    f = std::rotr(f, 19);
    f = f * 5 + 0xe6546b64;
    std::size_t iters = (len - 1) / 20;
    do
    {
        std::uint32_t a0 = std::rotr(Fetch32(s) * c1, 17) * c2;
        std::uint32_t a1 = Fetch32(s + 4);
        std::uint32_t a2 = std::rotr(Fetch32(s + 8) * c1, 17) * c2;
        std::uint32_t a3 = std::rotr(Fetch32(s + 12) * c1, 17) * c2;
        std::uint32_t a4 = Fetch32(s + 16);
        h ^= a0;
        h = std::rotr(h, 18);
        h = h * 5 + 0xe6546b64;
        f += a1;
        f = std::rotr(f, 19);
        f = f * c1;
        g += a2;
        g = std::rotr(g, 18);
        g = g * 5 + 0xe6546b64;
        h ^= a3 + a1;
        h = std::rotr(h, 19);
        h = h * 5 + 0xe6546b64;
        g ^= a4;
        g = ByteSwap32(g) * 5;
        h += a4 * 5;
        h = ByteSwap32(h);
        f += a0;
        Permute3(f, h, g);
        s += 20;
    } while (--iters != 0);
    g = std::rotr(g, 11) * c1;
    g = std::rotr(g, 17) * c1;
    f = std::rotr(f, 11) * c1;
    f = std::rotr(f, 17) * c1;
    h = std::rotr(h + g, 19);
    h = h * 5 + 0xe6546b64;
    h = std::rotr(h, 17) * c1;
    h = std::rotr(h + f, 19);
    h = h * 5 + 0xe6546b64;
    h = std::rotr(h, 17) * c1;
    return h;
}

static constexpr std::uint64_t ShiftMix(std::uint64_t val) noexcept
{
    return val ^ (val >> 47);
}

static constexpr std::uint64_t HashLen16(std::uint64_t u, std::uint64_t v) noexcept
{
    return Hash128to64(uint128(u, v));
}

static constexpr std::uint64_t HashLen16(std::uint64_t u, std::uint64_t v, std::uint64_t mul) noexcept
{
    // Murmur-inspired hashing.
    std::uint64_t a = (u ^ v) * mul;
    a ^= (a >> 47);
    std::uint64_t b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

static std::uint64_t HashLen0to16(const char* s, std::size_t len)
{
    if (len >= 8)
    {
        std::uint64_t mul = k2 + len * 2;
        std::uint64_t a = Fetch64(s) + k2;
        std::uint64_t b = Fetch64(s + len - 8);
        std::uint64_t c = std::rotr(b, 37) * mul + a;
        std::uint64_t d = (std::rotr(a, 25) + b) * mul;
        return HashLen16(c, d, mul);
    }
    if (len >= 4)
    {
        std::uint64_t mul = k2 + len * 2;
        std::uint64_t a = Fetch32(s);
        return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
    }
    if (len > 0)
    {
        std::uint8_t a = static_cast<std::uint8_t>(s[0]);
        std::uint8_t b = static_cast<std::uint8_t>(s[len >> 1]);
        std::uint8_t c = static_cast<std::uint8_t>(s[len - 1]);
        std::uint32_t y = static_cast<std::uint32_t>(a) + (static_cast<std::uint32_t>(b) << 8);
        std::uint32_t z = static_cast<std::uint32_t>(len) + (static_cast<std::uint32_t>(c) << 2);
        return ShiftMix(y * k2 ^ z * k0) * k2;
    }
    return k2;
}

// This probably works well for 16-byte strings as well, but it may be overkill
// in that case.
static std::uint64_t HashLen17to32(const char* s, std::size_t len)
{
    std::uint64_t mul = k2 + len * 2;
    std::uint64_t a = Fetch64(s) * k1;
    std::uint64_t b = Fetch64(s + 8);
    std::uint64_t c = Fetch64(s + len - 8) * mul;
    std::uint64_t d = Fetch64(s + len - 16) * k2;
    return HashLen16(std::rotr(a + b, 43) + std::rotr(c, 30) + d, a + std::rotr(b + k2, 18) + c, mul);
}

// Return a 16-byte hash for 48 bytes. Quick and dirty.
// Callers do best to use "random-looking" values for a and b.
static std::pair<std::uint64_t, std::uint64_t> WeakHashLen32WithSeeds(
    std::uint64_t w, std::uint64_t x, std::uint64_t y, std::uint64_t z, std::uint64_t a, std::uint64_t b)
{
    a += w;
    b = std::rotr(b + a + z, 21);
    std::uint64_t c = a;
    a += x;
    a += y;
    b += std::rotr(a, 44);
    return std::make_pair(a + z, b + c);
}

// Return a 16-byte hash for s[0] ... s[31], a, and b. Quick and dirty.
static std::pair<std::uint64_t, std::uint64_t> WeakHashLen32WithSeeds(const char* s, std::uint64_t a, std::uint64_t b)
{
    return WeakHashLen32WithSeeds(Fetch64(s), Fetch64(s + 8), Fetch64(s + 16), Fetch64(s + 24), a, b);
}

// Return an 8-byte hash for 33 to 64 bytes.
static std::uint64_t HashLen33to64(const char* s, std::size_t len)
{
    std::uint64_t mul = k2 + len * 2;
    std::uint64_t a = Fetch64(s) * k2;
    std::uint64_t b = Fetch64(s + 8);
    std::uint64_t c = Fetch64(s + len - 24);
    std::uint64_t d = Fetch64(s + len - 32);
    std::uint64_t e = Fetch64(s + 16) * k2;
    std::uint64_t f = Fetch64(s + 24) * 9;
    std::uint64_t g = Fetch64(s + len - 8);
    std::uint64_t h = Fetch64(s + len - 16) * mul;
    std::uint64_t u = std::rotr(a + g, 43) + (std::rotr(b, 30) + c) * 9;
    std::uint64_t v = ((a + g) ^ d) + f + 1;
    std::uint64_t w = ByteSwap64((u + v) * mul) + h;
    std::uint64_t x = std::rotr(e + f, 42) + c;
    std::uint64_t y = (ByteSwap64((v + w) * mul) + g) * mul;
    std::uint64_t z = e + f + c;
    a = ByteSwap64((x + z) * mul + y) + b;
    b = ShiftMix((z + a) * mul + d + h) * mul;
    return b + x;
}

std::uint64_t CityHash64(const char* s, std::size_t len)
{
    if (len <= 32)
    {
        if (len <= 16)
        {
            return HashLen0to16(s, len);
        }
        else
        {
            return HashLen17to32(s, len);
        }
    }
    else if (len <= 64)
    {
        return HashLen33to64(s, len);
    }

    // For strings over 64 bytes we hash the end first, and then as we
    // loop we keep 56 bytes of state: v, w, x, y, and z.
    std::uint64_t x = Fetch64(s + len - 40);
    std::uint64_t y = Fetch64(s + len - 16) + Fetch64(s + len - 56);
    std::uint64_t z = HashLen16(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
    std::pair<std::uint64_t, std::uint64_t> v = WeakHashLen32WithSeeds(s + len - 64, len, z);
    std::pair<std::uint64_t, std::uint64_t> w = WeakHashLen32WithSeeds(s + len - 32, y + k1, x);
    x = x * k1 + Fetch64(s);

    // Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
    len = (len - 1) & ~static_cast<std::size_t>(63);
    do
    {
        x = std::rotr(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = std::rotr(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = std::rotr(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        len -= 64;
    } while (len != 0);
    return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z, HashLen16(v.second, w.second) + x);
}

std::uint64_t CityHash64WithSeed(const char* s, std::size_t len, std::uint64_t seed)
{
    return CityHash64WithSeeds(s, len, k2, seed);
}

std::uint64_t CityHash64WithSeeds(const char* s, std::size_t len, std::uint64_t seed0, std::uint64_t seed1)
{
    return HashLen16(CityHash64(s, len) - seed0, seed1);
}

// A subroutine for CityHash128().
// Returns a decent 128-bit hash for strings of any length representable in signed long.
// Based on City and Murmur.
static uint128 CityMurmur(const char* s, std::size_t len, uint128 seed)
{
    std::uint64_t a = seed.first;
    std::uint64_t b = seed.second;
    std::uint64_t c = 0;
    std::uint64_t d = 0;
    if (len <= 16)
    {
        a = ShiftMix(a * k1) * k1;
        c = b * k1 + HashLen0to16(s, len);
        if (len >= 8)
        {
            d = ShiftMix(a + Fetch64(s));
        }
        else
        {
            d = ShiftMix(a + c);
        }
    }
    else
    {
        c = HashLen16(Fetch64(s + len - 8) + k1, a);
        d = HashLen16(b + len, c + Fetch64(s + len - 16));
        a += d;
        // len > 16 here, so do...while is safe
        do
        {
            a ^= ShiftMix(Fetch64(s) * k1) * k1;
            a *= k1;
            b ^= a;
            c ^= ShiftMix(Fetch64(s + 8) * k1) * k1;
            c *= k1;
            d ^= c;
            s += 16;
            len -= 16;
        } while (len > 16);
    }
    a = HashLen16(a, c);
    b = HashLen16(d, b);
    return uint128(a ^ b, HashLen16(b, a));
}

uint128 CityHash128WithSeed(const char* s, std::size_t len, uint128 seed)
{
    if (len < 128)
    {
        return CityMurmur(s, len, seed);
    }

    // We expect len >= 128 to be the common case.
    // Keep 56 bytes of state: v, w, x, y, and z.
    std::pair<std::uint64_t, std::uint64_t> v, w;
    std::uint64_t x = seed.first;
    std::uint64_t y = seed.second;
    std::uint64_t z = len * k1;
    v.first = std::rotr(y ^ k1, 49) * k1 + Fetch64(s);
    v.second = std::rotr(v.first, 42) * k1 + Fetch64(s + 8);
    w.first = std::rotr(y + z, 35) * k1 + x;
    w.second = std::rotr(x + Fetch64(s + 88), 53) * k1;

    // This is the same inner loop as CityHash64(), manually unrolled.
    do
    {
        x = std::rotr(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = std::rotr(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = std::rotr(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        x = std::rotr(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = std::rotr(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = std::rotr(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        len -= 128;
    } while (len >= 128);
    x += std::rotr(v.first + z, 49) * k0;
    y = y * k0 + std::rotr(w.second, 37);
    z = z * k0 + std::rotr(w.first, 27);
    w.first *= 9;
    v.first *= k0;
    // If 0 < len < 128, hash up to 4 chunks of 32 bytes each from the end of s.
    for (std::size_t tail_done = 0; tail_done < len;)
    {
        tail_done += 32;
        y = std::rotr(x + y, 42) * k0 + v.second;
        w.first += Fetch64(s + len - tail_done + 16);
        x = x * k0 + w.first;
        z += w.second + Fetch64(s + len - tail_done);
        w.second += v.first;
        v = WeakHashLen32WithSeeds(s + len - tail_done, v.first + z, v.second);
        v.first *= k0;
    }
    // At this point our 56 bytes of state should contain more than enough information for a strong 128-bit hash.
    // We use two different 56-byte-to-8-byte hashes to get a 16-byte final result.
    x = HashLen16(x, v.first);
    y = HashLen16(y + z, w.first);
    return uint128(HashLen16(x + v.second, w.second) + y, HashLen16(x + w.second, y + v.second));
}

uint128 CityHash128(const char* s, std::size_t len)
{
    if (len >= 16)
    {
        return CityHash128WithSeed(s + 16, len - 16, uint128(Fetch64(s), Fetch64(s + 8) + k0));
    }
    return CityHash128WithSeed(s, len, uint128(k0, k1));
}
