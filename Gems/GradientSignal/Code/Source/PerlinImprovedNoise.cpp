/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <GradientSignal/PerlinImprovedNoise.h>

#include <numeric>
#include <random> // std::mt19937 std::random_device


namespace GradientSignal
{
    //////////////////////////////////////////////////////////////////////////
    // http://flafla2.github.io/2014/08/09/perlinnoise.html
    // https://gist.github.com/Flafla2/f0260a861be0ebdeef76
    // MIT License, http://www.opensource.org/licenses/mit-license.php


    // Source: http://riven8192.blogspot.com/2010/08/calculate-perlinnoise-twice-as-fast.html
    namespace PerlinImprovedNoiseDetails
    {
        AZ_INLINE float Gradient(int hash, float x, float y, float z)
        {
            switch (hash & 0xF)
            {
            case 0x0: return  x + y;
            case 0x1: return -x + y;
            case 0x2: return  x - y;
            case 0x3: return -x - y;
            case 0x4: return  x + z;
            case 0x5: return -x + z;
            case 0x6: return  x - z;
            case 0x7: return -x - z;
            case 0x8: return  y + z;
            case 0x9: return -y + z;
            case 0xA: return  y - z;
            case 0xB: return -y - z;
            case 0xC: return  y + x;
            case 0xD: return -y + z;
            case 0xE: return  y - x;
            case 0xF: return -y - z;
            default: return 0; // never happens
            }
        }

        AZ_FORCE_INLINE float Fade(float t)
        {
            // Fade function as defined by Ken Perlin.  This eases coordinate values
            // so that they will "ease" towards integral values.  
            // This ends up smoothing the final output.
            return t * t * t * (t * (t * 6 - 15) + 10); // 6t^5 - 15t^4 + 10t^3
        }

        AZ_FORCE_INLINE float Lerp(float a, float b, float x)
        {
            return a + x * (b - a);
        }
    }

    PerlinImprovedNoise::PerlinImprovedNoise(int seed)
    {
        PrepareTable(seed);
    }

    PerlinImprovedNoise::PerlinImprovedNoise(const AZStd::span<int, 512>& permutationTable)
    {
        AZStd::copy(permutationTable.begin(), permutationTable.end(), m_permutationTable.begin());
    }

    float PerlinImprovedNoise::GenerateOctaveNoise(float x, float y, float z, int octaves, float persistence, float initialFrequency)
    {
        float total = 0.0f;
        float frequency = initialFrequency;
        float amplitude = 1.0f;
        float maxValue = 0.0f;               // Used for normalizing result to 0.0 - 1.0
        for (int i = 0; i < octaves; ++i)
        {
            total += GenerateNoise(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }
        if (maxValue <= 0.0f)
        {
            return 0.0f;
        }
        return total / maxValue;
    }

    float PerlinImprovedNoise::GenerateNoise(float x, float y, float z)
    {
        const int fx = (int)std::floor(x);
        const int fy = (int)std::floor(y);
        const int fz = (int)std::floor(z);
        const float xf = x - fx;                                // We also fade the location to smooth the result.
        const float yf = y - fy;
        const float zf = z - fz;
        const int xi0 = fx & 255;                               // Calculate the "unit cube" that the point asked will be located in
        const int yi0 = fy & 255;                               // The left bound is ( |_x_|,|_y_|,|_z_| ) and the right bound is that
        const int zi0 = fz & 255;                               // plus 1.  Next we calculate the location (from 0.0 to 1.0) in that cube.
        const int xi1 = (xi0 + 1) /*% 255*/;
        const int yi1 = (yi0 + 1) /*% 255*/;
        const int zi1 = (zi0 + 1) /*% 255*/;
        const float u = PerlinImprovedNoiseDetails::Fade(xf);
        const float v = PerlinImprovedNoiseDetails::Fade(yf);
        const float w = PerlinImprovedNoiseDetails::Fade(zf);

        const AZStd::array<int, 512>& p = m_permutationTable;

        const int aaa = p[p[p[xi0] + yi0] + zi0];
        const int aba = p[p[p[xi0] + yi1] + zi0];
        const int aab = p[p[p[xi0] + yi0] + zi1];
        const int abb = p[p[p[xi0] + yi1] + zi1];
        const int baa = p[p[p[xi1] + yi0] + zi0];
        const int bba = p[p[p[xi1] + yi1] + zi0];
        const int bab = p[p[p[xi1] + yi0] + zi1];
        const int bbb = p[p[p[xi1] + yi1] + zi1];

        // The gradient function calculates the dot product between a pseudorandom
        // gradient vector and the vector from the input coordinate to the 8
        // surrounding points in its unit cube. This is all then lerped together as a sort of 
        // weighted average based on the faded (u,v,w) values we made earlier.

        float x1, x2, y1, y2;
        x1 = PerlinImprovedNoiseDetails::Lerp(PerlinImprovedNoiseDetails::Gradient(aaa, xf, yf, zf), PerlinImprovedNoiseDetails::Gradient(baa, xf - 1.0f, yf, zf), u);
        x2 = PerlinImprovedNoiseDetails::Lerp(PerlinImprovedNoiseDetails::Gradient(aba, xf, yf - 1.0f, zf), PerlinImprovedNoiseDetails::Gradient(bba, xf - 1.0f, yf - 1.0f, zf), u);
        y1 = PerlinImprovedNoiseDetails::Lerp(x1, x2, v);
        x1 = PerlinImprovedNoiseDetails::Lerp(PerlinImprovedNoiseDetails::Gradient(aab, xf, yf, zf - 1.0f), PerlinImprovedNoiseDetails::Gradient(bab, xf - 1.0f, yf, zf - 1.0f), u);
        x2 = PerlinImprovedNoiseDetails::Lerp(PerlinImprovedNoiseDetails::Gradient(abb, xf, yf - 1.0f, zf - 1.0f), PerlinImprovedNoiseDetails::Gradient(bbb, xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
        y2 = PerlinImprovedNoiseDetails::Lerp(x1, x2, v);

        // For convenience we bound it to 0 - 1 (theoretical min/max before is -1 - 1)
        return (PerlinImprovedNoiseDetails::Lerp(y1, y2, w) + 1.0f) / 2.0f;
    }

    void PerlinImprovedNoise::PrepareTable(int seed)
    {
        AZStd::array<int, 256> randtable;
        std::iota(randtable.begin(), randtable.end(), 0);
        std::shuffle(randtable.begin(), randtable.end(), std::mt19937(seed));

        for (int x = 0; x < 256; ++x)
        {
            m_permutationTable[x] = randtable[x];
            m_permutationTable[x + 256] = randtable[x];
        }
    }
}
