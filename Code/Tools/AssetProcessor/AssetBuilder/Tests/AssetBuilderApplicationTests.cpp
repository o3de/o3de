/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AssetBuilderApplication.h>
#include <TraceMessageHook.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>

namespace AssetBuilder
{
    using namespace UnitTest;

    using AssetBuilderAppTest = LeakDetectionFixture;

    TEST_F(AssetBuilderAppTest, AssetBuilder_EditorScriptingComponents_Exists)
    {
        AssetBuilderApplication app(nullptr, nullptr);
        AZ::ComponentTypeList systemComponents = app.GetRequiredSystemComponents();

        auto searchFor = [&systemComponents](const AZ::Uuid& typeId) -> bool
        {
            auto entry = AZStd::find(systemComponents.begin(), systemComponents.end(), typeId);
            return systemComponents.end() != entry;
        };

        EXPECT_TRUE(searchFor(azrtti_typeid<AzToolsFramework::SliceMetadataEntityContextComponent>()));
        EXPECT_TRUE(searchFor(azrtti_typeid<AzToolsFramework::Components::EditorComponentAPIComponent>()));
        EXPECT_TRUE(searchFor(azrtti_typeid<AzToolsFramework::Components::EditorEntitySearchComponent>()));
        EXPECT_TRUE(searchFor(azrtti_typeid<AzToolsFramework::Components::EditorEntityModelComponent>()));
        EXPECT_TRUE(searchFor(azrtti_typeid<AzToolsFramework::EditorEntityContextComponent>()));
    }
    

    void VerifyOutput(const AZStd::string& output)
    {
        ASSERT_FALSE(output.empty());

        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(output, tokens, "\n", false, false);

        // There should be an even number of lines since every line has a context line printed before it
        ASSERT_GT(tokens.size(), 0);
        ASSERT_EQ(tokens.size() % 2, 0);

        for (int i = 0; i < tokens.size(); i += 2)
        {
            ASSERT_STREQ(tokens[0].c_str(), "C: [Source] = Test");
        }
    }

    struct LoggingTest
        : LeakDetectionFixture
    {
        void SetUp() override
        {
            m_messageHook.EnableTraceContext(true);
            
        }

        TraceMessageHook m_messageHook;
    };

    TEST_F(LoggingTest, TracePrintf_ContainsContextOnEachLine)
    {
        testing::internal::CaptureStdout();

        AZ_TraceContext("Source", "Test");
        AZ_TracePrintf("window", "line1\nline2\nline3");

        auto output = testing::internal::GetCapturedStdout();
        VerifyOutput(output.c_str());
    }

    TEST_F(LoggingTest, Warning_ContainsContextOnEachLine)
    {
        testing::internal::CaptureStdout();

        AZ_TraceContext("Source", "Test");
        AZ_Warning("window", false, "line1\nline2\nline3");

        auto output = testing::internal::GetCapturedStdout();
        VerifyOutput(output.c_str());
    }

    TEST_F(LoggingTest, Error_ContainsContextOnEachLine)
    {
        testing::internal::CaptureStderr();

        AZ_TraceContext("Source", "Test");
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ_Error("window", false, "line1\nline2\nline3");
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

        auto output = testing::internal::GetCapturedStderr();
        VerifyOutput(output.c_str());
    }

    TEST_F(LoggingTest, Assert_ContainsContextOnEachLine)
    {
        testing::internal::CaptureStderr();

        AZ_TraceContext("Source", "Test");
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ_Assert(false, "line1\nline2\nline3");
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

        auto output = testing::internal::GetCapturedStderr();
        VerifyOutput(output.c_str());
    }
} // namespace AssetBuilder
