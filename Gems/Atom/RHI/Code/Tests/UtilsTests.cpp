/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <AzFramework/IO/LocalFileIO.h>
#include <Atom/RHI.Edit/Utils.h>
#include <AzFramework/Application/Application.h>

namespace UnitTest
{
    class UtilsTests
        : public RHITestFixture
    {
    protected:

        UtilsTests()
            : m_application{ AZStd::make_unique<AzFramework::Application>() }
        {
        }

        ~UtilsTests() override
        {
            m_application.reset();
        }

        static constexpr const char TestDataFolder[] = "@engroot@/Gems/Atom/RHI/Code/Tests/UtilsTestsData/";

        AZStd::unique_ptr<AzFramework::Application> m_application;
    };

    TEST_F(UtilsTests, LoadFileString)
    {
        AZStd::string testFilePath = TestDataFolder + AZStd::string("HelloWorld.txt");
        AZ::Outcome<AZStd::string, AZStd::string> outcome = AZ::RHI::LoadFileString(testFilePath.c_str());
        EXPECT_TRUE(outcome.IsSuccess());
        auto& str = outcome.GetValue();
        str.erase(AZStd::remove(str.begin(), str.end(), '\r'));
        EXPECT_EQ(AZStd::string("Hello World!\n"), str);
    }

    TEST_F(UtilsTests, LoadFileBytes)
    {
        AZStd::string testFilePath = TestDataFolder + AZStd::string("HelloWorld.txt");
        AZ::Outcome<AZStd::vector<uint8_t>, AZStd::string> outcome = AZ::RHI::LoadFileBytes(testFilePath.c_str());
        EXPECT_TRUE(outcome.IsSuccess());
        AZStd::string expectedText = "Hello World!\n";
        auto& str = outcome.GetValue();
        str.erase(AZStd::remove(str.begin(), str.end(), '\r'));
        EXPECT_EQ(AZStd::vector<uint8_t>(expectedText.begin(), expectedText.end()), str);
    }

    TEST_F(UtilsTests, LoadFileString_Error_DoesNotExist)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto outcome = AZ::RHI::LoadFileString("FileDoesNotExist");
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_FALSE(outcome.IsSuccess());
        EXPECT_TRUE(outcome.GetError().find("Could not open file") != AZStd::string::npos);
        EXPECT_TRUE(outcome.GetError().find("FileDoesNotExist") != AZStd::string::npos);
    }

    TEST_F(UtilsTests, LoadFileBytes_Error_DoesNotExist)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto outcome = AZ::RHI::LoadFileBytes("FileDoesNotExist");
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_FALSE(outcome.IsSuccess());
        EXPECT_TRUE(outcome.GetError().find("Could not open file") != AZStd::string::npos);
        EXPECT_TRUE(outcome.GetError().find("FileDoesNotExist") != AZStd::string::npos);
    }

    TEST_F(UtilsTests, RegexCount_DXIL)
    {
        AZStd::string testFilePath = TestDataFolder + AZStd::string("DummyTransformColor.MainPS.dx12.dxil.txt");
        auto ojectCode = AZ::RHI::LoadFileString(testFilePath.c_str());
        EXPECT_TRUE(ojectCode.IsSuccess());
        size_t dynamicBranchCount = AZ::RHI::RegexCount(ojectCode.GetValue(), "^ *(br|indirectbr|switch) ");
        EXPECT_EQ(10, dynamicBranchCount);
    }

    TEST_F(UtilsTests, RegexCount_SPIRV)
    {
        AZStd::string testFilePath = TestDataFolder + AZStd::string("DummyTransformColor.MainPS.vulkan.spirv.txt");
        auto ojectCode = AZ::RHI::LoadFileString(testFilePath.c_str());
        EXPECT_TRUE(ojectCode.IsSuccess());
        size_t dynamicBranchCount = AZ::RHI::RegexCount(ojectCode.GetValue(), "^ *(OpBranch|OpBranchConditional|OpSwitch) ");
        EXPECT_EQ(23, dynamicBranchCount);
    }
}
