/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/UnitTest/TestTypes.h>


#if defined(HAVE_BENCHMARK)

#include "CrcTestsCompileTimeLiterals.h"

namespace Benchmark
{
    inline namespace Crc32Internal
    {
        //! The number of different Crc32 Values to calculate at compile
        //! This can be upped to check the Benchmarks
        constexpr size_t NumConstevalCrc32CompileValues = 1;
        using TestCrc32Array = AZStd::array<AZ::Crc32, NumConstevalCrc32CompileValues>;

        constexpr TestCrc32Array CreateCrc32FromLiteral(const AZStd::string_view (&testValues) [TestStringLiteralSize])
        {
            TestCrc32Array resultArray{};

            for (size_t crcIndex = 0; crcIndex < NumConstevalCrc32CompileValues; ++crcIndex)
            {
                resultArray[crcIndex] = AZ::Crc32(testValues[crcIndex % TestStringLiteralSize]);
            }
            return resultArray;
        }

        // Generates an array of Crc32 values based on the generated array of test string literals
        // The time it takes this function to compile is the amount of time spent creating Crc32 values
        // at compile time
        constexpr TestCrc32Array GenerateTestCrc32Values()
        {
            return CreateCrc32FromLiteral(TestStringLiterals);
        }
    }

    class Crc32BenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
    {};

    static Crc32BenchmarkEnvironment& s_crcBenchmarkEnv = AZ::Test::RegisterBenchmarkEnvironment<Crc32BenchmarkEnvironment>();

    static void MeasureCrc32ConstevalTime(::benchmark::State& state)
    {
        // Runtime performance is not actually being measured by this test.
        // This function only exist to calculate AZ::Crc32 values at compile time
        for ([[maybe_unused]] auto _ : state)
        {
            [[maybe_unused]] constexpr auto resultArray = Crc32Internal::GenerateTestCrc32Values();
        }
    }

    BENCHMARK(MeasureCrc32ConstevalTime);
}

#endif

namespace UnitTest
{
    class Crc32Fixture
        : public UnitTest::LeakDetectionFixture
    {

    };
    TEST_F(Crc32Fixture, Constructor_IsConstexpr)
    {
        static_assert(AZ::Crc32() == AZ::Crc32{ 0U }, "Default constructed Crc32 should 0");
        static_assert(AZ::Crc32(0x6dc044c5) == AZ::Crc32(0x6dc044c5), R"(Crc32 should match the calculation on the string "group")");
        static_assert(AZ::Crc32("EditorData") == AZ::Crc32(0xf44f1a1d), R"(Crc32 should match the calculation on the string "editordata")");
        // Crc value for lowercase string of "group"
        static_assert(AZ::Crc32(AZStd::string_view{ "EditorData+RuntimeData", 10 }) == AZ::Crc32(0xf44f1a1d), R"(Crc32 should match the calculation on the string "editordata")");
        static_assert(AZ::Crc32("Editor", 6,  true) == AZ::Crc32(0xccf1f1ba), R"(Crc32 should match the calculation on the string "editor")");
        static_assert(AZ::Crc32("Editor", 6,  false) == AZ::Crc32(0xcb5df48c), R"(Crc32 should match the calculation on the string "Editor")");

        constexpr uint8_t binaryData[] = { 'B', 'i', 'n', 0x3 };
        static_assert(AZ::Crc32(binaryData, 4, false) == AZ::Crc32(0x3528a896), R"(Crc32 should match the calculation on the binary blob "Bin\x03")");
        constexpr AZStd::byte byteData[]{ AZStd::byte('B'), AZStd::byte('i'), AZStd::byte('n'), AZStd::byte(0x3) };
        static_assert(AZ::Crc32(AZStd::span(byteData)) == AZ::Crc32(0x3528a896));
    }

    TEST_F(Crc32Fixture, OperatorUint32t_IsConstexpr)
    {
        static_assert(static_cast<AZ::u32>(AZ::Crc32("EditorData")) == 0xf44f1a1d, R"(Crc32 should match the calculation on the string "editordata")");
    }

    TEST_F(Crc32Fixture, Add_IsConstexpr)
    {
        constexpr auto TestAdd = []() constexpr -> AZ::Crc32
        {
            AZ::Crc32 hello("Hello");
            hello.Add(" World");
            return hello;
        };
        constexpr AZ::Crc32 addResult = TestAdd();
        static_assert(addResult == AZ::Crc32(0x0d4a1185), R"(Crc32 Add function result is unexpected)");
        EXPECT_EQ(AZ::Crc32(0x0d4a1185), addResult);
    }

    TEST_F(Crc32Fixture, CrcConstevalMacro_IsConstexpr)
    {
        AZ::Crc32 constEvalLiteralValue = AZ_CRC_CE("Hello");

        static_assert(AZ_CRC_CE("Hello") == AZ::Crc32(0x3610a686));
        EXPECT_EQ(AZ::Crc32(0x3610a686), constEvalLiteralValue);

        AZ::Crc32 constEvalIntValue = AZ_CRC_CE(0x4727dc92);
        static_assert(AZ_CRC_CE(0x4727dc92) == AZ::Crc32(0x4727dc92));
        EXPECT_EQ(AZ::Crc32(0x4727dc92), constEvalIntValue);
    }

}
