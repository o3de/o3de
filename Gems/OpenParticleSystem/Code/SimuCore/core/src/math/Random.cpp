/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/math/Random.h"
#include "core/math/Math.h"
#include "core/platform/Platform.h"

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#else
#include <cstdio>
#endif

namespace SimuCore {
    int32_t Random::randSeed = 0;
    int32_t Random::initSeed = 0;

#if defined(_WIN32)
    bool Random::GenRandom(void* data, uint32_t size)
    {
        int res;
        HCRYPTPROV crypt;
        res = CryptAcquireContext(&crypt, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
        if (!res) {
            return false;
        }
        res = CryptGenRandom(crypt, static_cast<DWORD>(size), static_cast<PBYTE>(data));
        CryptReleaseContext(crypt, 0);
        if (!res) {
            return false;
        }
        return true;
    }
#else
    bool Random::GenRandom(void* data, uint32_t size)
    {
        size_t res;
        auto *fp = fopen("/dev/urandom", "rb");
        if (fp == nullptr) {
            return false;
        }
        res = fread(data, 1, size, fp);
        (void)fclose(fp);
        if (res != size) {
            return false;
        }
        return true;
    }
#endif

    namespace internal {
        union FloatConvert {
            float vf;
            uint32_t vi;
        };
    }

    void Random::RandSeed(int32_t seed)
    {
        randSeed = seed;
        initSeed = seed;
    }

    void Random::MutateSeed(int32_t& seed)
    {
        // the fixed algorithm of random seed
        seed = (seed * 196314165) + 907633515;
    }

    float Random::Rand(int32_t seed)
    {
        internal::FloatConvert tmp;
        internal::FloatConvert result;
        tmp.vf = 1.0f;
        result.vi = (tmp.vi & 0xff800000) | (static_cast<uint32_t>(seed) & 0x007fffff);
        return Math::Fractional(result.vf);
    }

    float Random::Rand()
    {
        if (initSeed == 0) {
            (void)GenRandom(initSeed);
            randSeed = initSeed;
        }

        MutateSeed(randSeed);
        return Rand(randSeed);
    }

    float Random::RandomRange(float min, float max)
    {
        return min + (max - min) * Rand();
    }

    uint32_t Random::RandomRange(uint32_t min, uint32_t max)
    {
        return min + static_cast<uint32_t>(static_cast<float>(max - min) * Rand());
    }

    AZ::Color Random::RandomRange(const AZ::Color& min, const AZ::Color& max)
    {
        return AZ::Color(min.GetR() + Rand() * (max.GetR() - min.GetR()),
                         min.GetG() + Rand() * (max.GetG() - min.GetG()),
                         min.GetB() + Rand() * (max.GetB() - min.GetB()),
                         min.GetA() + Rand() * (max.GetA() - min.GetA()));
    }
}