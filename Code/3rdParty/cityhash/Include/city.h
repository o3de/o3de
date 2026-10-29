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
// http://code.google.com/p/cityhash/
//
// This file provides a few functions for hashing strings.  All of them are
// high-quality functions in the sense that they pass standard tests such
// as Austin Appleby's SMHasher.  They are also fast.
//
// For 64-bit x86 code, on short strings, we don't know of anything faster than
// CityHash64 that is of comparable quality.  We believe our nearest competitor
// is Murmur3.  For 64-bit x86 code, CityHash64 is an excellent choice for hash
// tables and most other hashing (excluding cryptography).
//
// For 32-bit x86 code, we don't know of anything faster than CityHash32 that
// is of comparable quality.  We believe our nearest competitor is Murmur3A.
// (On 64-bit CPUs, it is typically faster to use the other CityHash variants.)
//
// Functions in the CityHash family are not suitable for cryptography.
//
// Please see CityHash's README file for more details on our performance
// measurements and so on.
//
// WARNING: This code has been only lightly tested on big-endian platforms!
// It is known to work well on little-endian platforms that have a small penalty
// for unaligned reads, such as current Intel and AMD moderate-to-high-end CPUs.
// It should work on all 32-bit and 64-bit platforms that allow unaligned reads;
// bug reports are welcome.
//
// By the way, for some hash functions, given strings a and b, the hash
// of a+b is easily derived from the hashes of a and b.  This property
// doesn't hold for any hash functions in this file.

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

using uint128 = std::pair<std::uint64_t, std::uint64_t>;

// Hash function for a byte array.
std::uint64_t CityHash64(const char* buf, std::size_t len);

// Hash function for a byte array.
// For convenience, a 64-bit seed is also hashed into the result.
std::uint64_t CityHash64WithSeed(const char* buf, std::size_t len, std::uint64_t seed);

// Hash function for a byte array.
// For convenience, two seeds are also hashed into the result.
std::uint64_t CityHash64WithSeeds(const char* buf, std::size_t len, std::uint64_t seed0, std::uint64_t seed1);

// Hash function for a byte array.
uint128 CityHash128(const char* s, std::size_t len);

// Hash function for a byte array.
// For convenience, a 128-bit seed is also hashed into the result.
uint128 CityHash128WithSeed(const char* s, std::size_t len, uint128 seed);

// Hash function for a byte array. Most useful in 32-bit binaries.
std::uint32_t CityHash32(const char* buf, std::size_t len);

// Hash 128 input bits down to 64 bits of output.
// This is intended to be a reasonably good hash function.
constexpr std::uint64_t Hash128to64(const uint128& x) noexcept
{
    // Murmur-inspired hashing.
    constexpr std::uint64_t kMul = 0x9ddfea08eb382d69ULL;
    std::uint64_t a = (x.first ^ x.second) * kMul;
    a ^= (a >> 47);
    std::uint64_t b = (x.second ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}
