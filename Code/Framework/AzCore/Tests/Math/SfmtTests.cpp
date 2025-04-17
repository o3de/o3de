/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Sfmt.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/parallel/semaphore.h>

using namespace AZ;

namespace UnitTest
{
    class MATH_SfmtTest
        : public LeakDetectionFixture
    {
        static const int BLOCK_SIZE = 100000;
        static const int BLOCK_SIZE64 = 50000;
        static const int COUNT = 1000;

        AZ::u64* array1;
        AZ::u64* array2;

    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            array1 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (BLOCK_SIZE / 4), AZStd::alignment_of<AZ::Vector4>::value);
            array2 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (10000 / 4), AZStd::alignment_of<AZ::Vector4>::value);
        }

        void TearDown() override
        {
            azfree(array1);
            azfree(array2);

            LeakDetectionFixture::TearDown();
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

        template<size_t NumThreads, size_t NumResultsPerThread, typename ResultType>
        bool ParallelTest()
        {
            Sfmt sfmt;

            // Arbitrary but deterministic seed values to guarantee that we get the same random numbers produced every time.
            constexpr AZ::u32 buffer[32] = {
                0x8236ed73, 0x854aa369, 0xc7b68864, 0x5f1e49da, 0x58ea5a08, 0xa33fc74b, 0x0336bd81, 0x8d3c11ac,
                0x7312bc57, 0x92fee3d1, 0x4b852f9f, 0xa7ac02ad, 0x4d72fb7a, 0x630641c6, 0x1edbeebf, 0xc18fdabe,
                0xc6588dba, 0x6a821cf5, 0xec7e24b3, 0x44246d7e, 0x24af2f32, 0x4f64d44c, 0x44b24116, 0x65585572,
                0x0f95038d, 0xd3e4c521, 0x6eea6bc1, 0x6d651ab5, 0x25dfc39e, 0x7502d183, 0xd32fc9bf, 0x9854093f };

            // Seed the sfmt generator.
            sfmt.Seed(buffer, AZ_ARRAY_SIZE(buffer));

            // Create a single array that will hold all the results, but we'll partition it so that each thread will write
            // its results to a different subset.
            AZStd::array<ResultType, NumResultsPerThread * NumThreads> results;
            AZStd::thread threads[NumThreads];

            // Use a semaphore to block thread execution until all the threads are created so that we can synchronize
            // their start times to the best of our ability.
            AZStd::semaphore startSync;

            // Spawn a set of threads that will each loop through and call the RandMethod (Rand32 or Rand64) for
            // a given number of times.
            for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
            {
                const size_t startOffset = NumResultsPerThread * threadIdx;

                auto threadFunc = [&startSync, startOffset, &results, &sfmt]()
                {
                    // Block so that we can start all the threads at the same time after creation.
                    startSync.acquire();

                    for (int i = 0; i < NumResultsPerThread; ++i)
                    {
                        // Call Rand32 when uint32_t is the passed-in type, and Rand64 if uint64_t is the passed-in type.
                        if constexpr (AZStd::is_same_v<ResultType, uint32_t>)
                        {
                            results[startOffset + i] = sfmt.Rand32();
                        }
                        else
                        {
                            results[startOffset + i] = sfmt.Rand64();
                        }
                    }
                };
                threads[threadIdx] = AZStd::thread(threadFunc);
            }

            // Now that all the threads are created, signal them to start.
            startSync.release(NumThreads);

            // Wait for all the threads to complete.
            for (auto& thread : threads)
            {
                thread.join();
            }

            // Sort the results from all the threads so that we can easily detect duplicates.
            AZStd::sort(results.begin(), results.end());

            // Look for duplicates and stop if we find one, since that's a full test failure. There's no need to keep iterating.
            for (size_t idx = 1; idx < results.size(); idx++)
            {
                if (results[idx - 1] == results[idx])
                {
                    EXPECT_NE(results[idx - 1], results[idx]);
                    return false;
                }
            }

            // Test was successful
            return true;
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
        // This test verifies that we can call Rand32() successfully in parallel.
        // It also performs a reasonable verification that we haven't reintroduced a race condition in which the random number
        // sequence could get repeated for periods of time if multiple threads hit the point where Sfmt refills its number cache
        // simultaneously. Because it was a race condition, this test isn't foolproof, but the test conditions set below were
        // causing a near-100% failure rate with the previous race condition and has a 100% success rate with the fix.

        // The following numbers were chosen to be high enough to expose the problem at least once per run but low enough to keep
        // the overall test runtime down.
        constexpr size_t NumIterations = 5;
        constexpr size_t NumThreads = 8;
        constexpr size_t NumResultsPerThread = 2000;

        // Run the test single-threaded once to verify that we don't have any numerical collisions in our test set and test size.
        bool testSetHasNoNumericalCollisions = ParallelTest<1, NumThreads * NumResultsPerThread, uint32_t>();
        ASSERT_TRUE(testSetHasNoNumericalCollisions);

        // Now run multi-threaded across multiple iterations to verify that we don't hit the race condition.
        for (size_t iterations = 0; iterations < NumIterations; iterations++)
        {
            bool parallelHasNoNumericalCollisions = ParallelTest<NumThreads, NumResultsPerThread, uint32_t>();
            ASSERT_TRUE(parallelHasNoNumericalCollisions);
        }
    }

    TEST_F(MATH_SfmtTest, TestParallel64)
    {
        // This test verifies that we can call Rand64() successfully in parallel.

        // The following numbers were chosen to be high enough to expose the problem at least once per run but low enough to keep
        // the overall test runtime down.
        constexpr size_t NumIterations = 5;
        constexpr size_t NumThreads = 8;
        constexpr size_t NumResultsPerThread = 2000;

        // Run the test single-threaded once to verify that we don't have any numerical collisions in our test set and test size.
        bool testSetHasNoNumericalCollisions = ParallelTest<1, NumThreads * NumResultsPerThread, uint64_t>();
        ASSERT_TRUE(testSetHasNoNumericalCollisions);

        // Now run multi-threaded across multiple iterations to verify that we don't hit the race condition.
        for (size_t iterations = 0; iterations < NumIterations; iterations++)
        {
            bool parallelHasNoNumericalCollisions = ParallelTest<NumThreads, NumResultsPerThread, uint64_t>();
            ASSERT_TRUE(parallelHasNoNumericalCollisions);
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
