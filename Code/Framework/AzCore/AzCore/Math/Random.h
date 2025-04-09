/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Math/Random_Platform.h>
#include <AzCore/Math/SimdMath.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    /**
     * A very simple and fast LCG random number generator, useful if you need a fast pseudo-random sequence and
     * don't really care about the quality of the sequence.
     * Based on the java Random class, since it's decent quality for a LCG, e.g. avoids cycles in the lower bits.
     * See http://download.oracle.com/javase/6/docs/api/java/util/Random.html for details, also a good reference for
     * some subtle issues.
     */
    class SimpleLcgRandom
    {
    public:
        AZ_TYPE_INFO(SimpleLcgRandom, "{52BD3FA7-C15D-41FA-BC85-0FD4D310F688}")

        SimpleLcgRandom(u64 seed = 1234) { SetSeed(seed); }

        void SetSeed(u64 seed)
        {
            m_seed = (seed ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
        }

        unsigned int GetRandom()
        {
            return static_cast<unsigned int>(Getu64Random() >> 16);
        }

        u64 Getu64Random()
        {
            m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            return m_seed;
        }

        //Gets a random float in the range [0,1)
        float GetRandomFloat()
        {
            unsigned int r = GetRandom();
            r &= 0x007fffff; //sets mantissa to random bits
            r |= 0x3f800000; //result is in [1,2), uniformly distributed
            union
            {
                float f;
                unsigned int i;
            } u;
            u.i = r;
            return u.f - 1.0f;
        }

    private:
        u64 m_seed;
    };

    //! This class is a vectorized implementation of LCG, allowing for the generation of 4 random values in parallel
    class SimpleLcgRandomVec4
    {
    public:
        AZ_TYPE_INFO(SimpleLcgRandomVec4, "{1E24CCC8-A1CC-49FF-AFAB-D87C0104F3B7}")

        SimpleLcgRandomVec4()
        {
            SetSeed(Simd::Vec4::LoadImmediate(rand(), rand(), rand(), rand()));
        }

        void SetSeed(Simd::Vec4::Int32ArgType seed)
        {
            const Simd::Vec4::Int32Type mask = Simd::Vec4::Splat(int32_t(0x7FFFFFFF)); // 2^31 - 1
            m_seed = Simd::Vec4::And(seed, mask);
        }

        // Gets four random ints in the range [0, 2^31)
        Simd::Vec4::Int32Type GetRandomInt4()
        {
            const Simd::Vec4::Int32Type scalar = Simd::Vec4::Splat(1103515245);
            const Simd::Vec4::Int32Type constant = Simd::Vec4::Splat(12345);
            const Simd::Vec4::Int32Type mask = Simd::Vec4::Splat(int32_t(0x7FFFFFFF)); // 2^31 - 1
            m_seed = Simd::Vec4::And(Simd::Vec4::Madd(m_seed, scalar, constant), mask);
            return m_seed;
        }

        // Gets four random floats in the range [0,1)
        Simd::Vec4::FloatType GetRandomFloat4()
        {
            Simd::Vec4::Int32Type randVal = GetRandomInt4();
            randVal = Simd::Vec4::And(randVal, Simd::Vec4::Splat(0x007fffff)); // Sets mantissa to random bits
            randVal = Simd::Vec4::Or(randVal, Simd::Vec4::Splat(0x3f800000)); // Result is in [1,2), uniformly distributed
            return Simd::Vec4::Sub(Simd::Vec4::CastToFloat(randVal), Simd::Vec4::Splat(1.0f));
        }

    private:
        Simd::Vec4::Int32Type m_seed;
    };

    /**
     * As an attempt to improve the pseudo-random (rand, time) we use a better random.
     * The cost of computing one if making it to great for high performance application, but
     * it's a great tool to get a good seed.
     */
    using BetterPseudoRandom = BetterPseudoRandom_Platform;

    /**
    * An enum representing the different random distributions available
    *
    * These map to random distributions in std::random. Those distributions
    * have no common base class so this is an enum to help determine which
    * random distribution is needed.
    */
    enum class RandomDistributionType : AZ::u32
    {
        Normal,
        UniformReal
    };

    //! Halton sequences are deterministic, quasi-random sequences with low discrepancy. They
    //! are useful for generating evenly distributed points.
    //! See https://en.wikipedia.org/wiki/Halton_sequence for more information.

    //! Returns a single halton number.
    //! @param index The index of the number. Indices start at 1. Using index 0 will return 0.
    //! @param base The numerical base of the halton number.
    inline float GetHaltonNumber(uint32_t index, uint32_t base)
    {
        float fraction = 1.0f;
        float result = 0.0f;

        while (index > 0)
        {
            fraction = fraction / base;
            result += fraction * (index % base);
            index = aznumeric_cast<uint32_t>(index / base);
        }

        return result;
    }

    //! A helper class for generating arrays of Halton sequences in n dimensions.
    //! The class holds the state of which bases to use, the starting offset
    //! of each dimension and how much to increment between each index for each
    //! dimension.
    template <uint8_t Dimensions>
    class HaltonSequence
    {
    public:

        //! Initializes a Halton sequence with some bases. By default there is no
        //! offset and the index increments by 1 between each number.
        HaltonSequence(AZStd::array<uint32_t, Dimensions> bases)
            : m_bases(bases)
        {
            m_offsets.fill(1); // Halton sequences start at index 1.
            m_increments.fill(1); // By default increment by 1 between each number.
        }
        
        //! Fills a provided container from begin to end with a Halton sequence.
        //! Entries are expected to be, or implicitly converted to, AZStd::array<float, Dimensions>.
        template<typename Iterator>
        void FillHaltonSequence(Iterator begin, Iterator end)
        {
            AZStd::array<uint32_t, Dimensions> indices = m_offsets;

            // Generator that returns the Halton number for all bases for a single entry.
            auto f = [&]()
            {
                AZStd::array<float, Dimensions> item;
                for (auto d = 0; d < Dimensions; ++d)
                {
                    item[d] = GetHaltonNumber(indices[d], m_bases[d]);
                    indices[d] += m_increments[d];
                }
                return item;
            };

            AZStd::generate(begin, end, f);
        }
        
        //! Returns a Halton sequence in an array of N length.
        template<uint32_t N>
        AZStd::array<AZStd::array<float, Dimensions>, N> GetHaltonSequence()
        {
            AZStd::array<AZStd::array<float, Dimensions>, N> result;
            FillHaltonSequence(result.begin(), result.end());
            return result;
        }

        //! Sets the offsets per dimension to start generating a sequence from.
        //! By default, there is no offset (offset of 0 corresponds to starting at index 1).
        void SetOffsets(AZStd::array<uint32_t, Dimensions> offsets)
        {
            m_offsets = offsets;

            // Halton sequences start at index 1, so increment all the indices.
            AZStd::for_each(m_offsets.begin(), m_offsets.end(), [](uint32_t &n){ n++; });
        }

        //! Sets the increment between numbers in the halton sequence per dimension
        //! By default this is 1, meaning that no numbers are skipped. Can be negative
        //! to generate numbers in reverse order.
        void SetIncrements(AZStd::array<int32_t, Dimensions> increments)
        {
            m_increments = increments;
        }

    private:

        AZStd::array<uint32_t, Dimensions> m_bases;
        AZStd::array<uint32_t, Dimensions> m_offsets;
        AZStd::array<int32_t, Dimensions> m_increments;

    };
}
