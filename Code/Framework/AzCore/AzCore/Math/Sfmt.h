/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/typetraits/static_storage.h>
#include <AzCore/Math/Internal/MathTypes.h>

namespace AZ
{
    class Sfmt;
    namespace SfmtInternal
    {
        //
        static const int MEXP = 19937;
        static const int N    = MEXP / 128 + 1;

        union W128_T
        {
            Simd::Vec4::Int32Type si;
            uint32_t u[4];
        };

        /** 128-bit data type */
        typedef W128_T w128_t;
        static_assert(N == MEXP / (sizeof(W128_T) * 8) + 1, "The m_smft member array must fit all iterations of the correct 128-bit size.");

        void gen_rand_all(Sfmt& g);
        void gen_rand_array(Sfmt& g, SfmtInternal::w128_t* array, int size);
    }

    /**
     * \file SFMT.h
     *
     * \brief SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom
     * number generator
     *
     * author Mutsuo Saito (Hiroshima University)
     * \author Makoto Matsumoto (Hiroshima University)
     *
     * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
     * University. All rights reserved.
     *
     * The new BSD License is applied to this software.
     * see LICENSE.txt
     */
    class Sfmt
    {
        friend void SfmtInternal::gen_rand_all(Sfmt& g);
        friend void SfmtInternal::gen_rand_array(Sfmt& g, SfmtInternal::w128_t* array, int size);
    public:
        /// By default we seed with Seed()
        Sfmt();
        /// Seed the generator with user defined seed Seed(AZ::u32* keys, int numKeys)
        Sfmt(const AZ::u32* keys, int numKeys);

        /// Seed the generator, with the best pseudo-random number the system can generate \ref BetterPseudoRandom
        void Seed();
        /// Seed the generator with user defined seed.
        void Seed(const AZ::u32* keys, int numKeys);

        /// Return u32 pseudo random integer
        AZ::u32 Rand32();
        /// Return u64 pseudo random integer. IMPORTANT: You can't call Rand64 after Rand32 (you need 2x Rand32).
        AZ::u64 Rand64();

        /// Fill an array with u32 pseudo random integers. array size MUST be > GetMinArray32Size() and a multiple of 4 (16 bytes since we use SIMD).
        void FillArray32(AZ::u32* array, int size);
        /// Fill an array with u64 pseudo random integers. array size MUST be > GetMinArray64Size() and a multiple of 2 (16 bytes since we use SIMD).
        void FillArray64(AZ::u64* array, int size);

        /// returns [0,1]
        inline double  RandR32()            { return Rand32() * (1.0 / 4294967295.0); }
        /// returns [0,1)
        inline double  RandR32_1()          { return Rand32() * (1.0 / 4294967296.0); }
        /// returns (0,1)
        inline double  RandR32_2()          { return (((double)Rand32()) + 0.5) * (1.0 / 4294967296.0); }

        /// Get the minimum array 32 size for FillArray32 function.
        int     GetMinArray32Size() const;
        /// Get the minimum array 32 size for FillArray64 function.
        int     GetMinArray64Size() const;


        /**
         * Returns the default global instance of the Sfmt, initialized with time(NULL) as seed. We recommend
         * creating your own instances when you need a big set of random numbers.
         */
        static Sfmt&     GetInstance();

        //! Creates a global instance of Smft using AZ::EnvironmentVariable
        static void Create();
        //! Releases a global instance of Smft
        static void Destroy();

    protected:

        void    PeriodCertification();

        SfmtInternal::w128_t        m_sfmt[SfmtInternal::N];
        size_t                      m_index = 0;         ///  Index into the pre-generated tables
        AZ::u32*                    m_psfmt32 = nullptr; ///  Read only tables of pre-generated random numbers
        AZ::u64*                    m_psfmt64 = nullptr;

        AZStd::mutex                m_sfmtMutex;         /// Guards access to m_index and m_sfmt
    };
}
