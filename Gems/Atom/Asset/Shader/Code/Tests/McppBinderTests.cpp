/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "Common/ShaderBuilderTestFixture.h"

#include <CommonFiles/Preprocessor.h>

namespace UnitTest
{
    using namespace AZ;

    // The main purpose of this class is to test ShaderBuilder::McppBinder::Fprintf_StaticHinge()
    // Which has three common scenarios to validate.
    // 1- The formatted string is expected to yield less bytes than McppBinder::DefaultFprintfBufferSize.
    // 2- The formatted string is expected to yield exactly McppBinder::DefaultFprintfBufferSize number of bytes.
    // 3- The formatted string is expectedc to yield more bytes than McppBinder::DefaultFprintfBufferSize.
    class McppBinderTests : public ShaderBuilderTestFixture
    {
    public:

        // Fills @buffer with 'a' to 'z' for up to @bufferSize number of bytes.
        // This function will null('\0') char terminate @buffer.
        void FillBufferWithAlphabet(char* buffer, int bufferSize)
        {
            for (int bufferPos = 0, rollback = 0; bufferPos < (bufferSize - 1); ++bufferPos)
            {
                const char value = 'a' + static_cast<char>(rollback++);
                buffer[bufferPos] = value;
                if (value == 'z')
                {
                    rollback = 0;
                }
            }
            buffer[bufferSize - 1] = '\0';
        }

        // Pushes the null terminated string, @inputString, into McppBinder capture stream
        // using McppBinder::Fprintf_StaticHinge().
        // Returns the content of the McppBinder capture stream as a string.
        AZStd::string PrintStringThroughStaticHinge(const char* inputString)
        {
            ShaderBuilder::PreprocessorData preprocessorData;
            ShaderBuilder::McppBinder mcppBinder(preprocessorData, false);
            ShaderBuilder::McppBinder::Fprintf_StaticHinge(MCPP_OUTDEST::MCPP_OUT, "%s", inputString);
            // convert from std::ostringstring to AZStd::string
            return AZStd::string(mcppBinder.m_outStream.str().c_str());
        }
    }; // class McppBinderTests


    TEST_F(McppBinderTests, ShouldPrintLessBytesThanDefaultSize)
    {
        constexpr int bufferSize = (ShaderBuilder::McppBinder::DefaultFprintfBufferSize / 2) + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }

    TEST_F(McppBinderTests, ShouldPrintSameBytesAsDefaultSize)
    {
        constexpr int bufferSize = ShaderBuilder::McppBinder::DefaultFprintfBufferSize + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }

    TEST_F(McppBinderTests, ShouldPrintMoreBytesThanDefaultSize)
    {
        constexpr int bufferSize = (ShaderBuilder::McppBinder::DefaultFprintfBufferSize * 2) + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }

} //namespace UnitTest

//AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

