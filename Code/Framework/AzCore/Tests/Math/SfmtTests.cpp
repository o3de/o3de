/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Sfmt.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    class MATH_SfmtTest
        : public AllocatorsFixture
    {
        static const int BLOCK_SIZE = 100000;
        static const int BLOCK_SIZE64 = 50000;
        static const int COUNT = 1000;

        AZ::u64* array1;
        AZ::u64* array2;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            array1 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (BLOCK_SIZE / 4), AZStd::alignment_of<AZ::u64>::value);
            array2 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (10000 / 4), AZStd::alignment_of<AZ::u64>::value);
        }

        void TearDown() override
        {
            azfree(array1);
            azfree(array2);

            AllocatorsFixture::TearDown();
        }

        void check32()
        {
            int i;
            AZ::u32* array32 = (AZ::u32*)array1;
            AZ::u32* array32_2 = (AZ::u32*)array2;
            AZ::u32 ini[] = { 0x1234, 0x5678, 0x9abc, 0xdef0 };

            Sfmt g;
            EXPECT_LT(g.GetMinArray32Size(), 10000);
            g.FillArray32(array32, 10000);
            g.FillArray32(array32_2, 10000);
            g.Seed();
            for (i = 0; i < 10000; i++)
            {
                EXPECT_NE(array32[i], g.Rand32());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_NE(array32_2[i], g.Rand32());
            }
            g.Seed(ini, 4);
            g.FillArray32(array32, 10000);
            g.FillArray32(array32_2, 10000);
            g.Seed(ini, 4);
            for (i = 0; i < 10000; i++)
            {
                EXPECT_EQ(array32[i], g.Rand32());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_EQ(array32_2[i], g.Rand32());
            }
        }

        void check64()
        {
            int i;
            AZ::u64* array64 = (AZ::u64*)array1;
            AZ::u64* array64_2 = (AZ::u64*)array2;
            AZ::u32 ini[] = { 5, 4, 3, 2, 1 };

            Sfmt g;
            EXPECT_LT(g.GetMinArray64Size(), 5000);
            g.FillArray64(array64, 5000);
            g.FillArray64(array64_2, 5000);
            g.Seed();
            for (i = 0; i < 5000; i++)
            {
                EXPECT_NE(array64[i], g.Rand64());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_NE(array64_2[i], g.Rand64());
            }
            g.Seed(ini, 5);
            g.FillArray64(array64, 5000);
            g.FillArray64(array64_2, 5000);
            g.Seed(ini, 5);
            for (i = 0; i < 5000; i++)
            {
                EXPECT_EQ(array64[i], g.Rand64());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_EQ(array64_2[i], g.Rand64());
            }
        }
    };

    TEST_F(MATH_SfmtTest, Test32Bit)
    {
        check32();
    }

    TEST_F(MATH_SfmtTest, Test64Bit)
    {
        check64();
    }

    TEST_F(MATH_SfmtTest, TestParallel32)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                sfmt.Rand32();
            }
        };
        AZStd::thread threads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(MATH_SfmtTest, TestParallel64)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                sfmt.Rand64();
            }
        };
        AZStd::thread threads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(MATH_SfmtTest, TestParallelInterleaved)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                AZ::u64 roll = sfmt.Rand64();
                if (roll % 2 == 0)
                {
                    sfmt.Rand32();
                }
                else
                {
                    sfmt.Rand64();
                }
            }
        };
        AZStd::thread threads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}
