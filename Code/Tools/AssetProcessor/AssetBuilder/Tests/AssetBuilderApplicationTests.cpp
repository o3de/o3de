/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#include <AssetBuilderApplication.h>
#include <TraceMessageHook.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>

namespace AssetBuilder
{
    using namespace UnitTest;

    using AssetBuilderAppTest = AllocatorsFixture;

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppNoArgs_NoExtraction)
    {
        AssetBuilderApplication app(nullptr, nullptr);

        char appRootBuffer[AZ_MAX_PATH_LEN];
        ASSERT_FALSE(app.GetOptionalAppRootArg(appRootBuffer, AZ_MAX_PATH_LEN));
    }

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppQuotedAppRoot_Success)
    {
    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* appRootArg = R"str(-approot="C:\path\to\app\root\")str";
        const char* expectedResult = R"str(C:\path\to\app\root\)str";
    #else
        const char* appRootArg = R"str(-approot="/path/to/app/root")str";
        const char* expectedResult = R"str(/path/to/app/root/)str";
    #endif

        const char* argArray[] = {
            appRootArg
        };
        int argc = AZ_ARRAY_SIZE(argArray);
        char** argv = const_cast<char**>(argArray); // this is unfortunately necessary to get around osx's strict non-const string literal stance 

        AssetBuilderApplication app(&argc, &argv);

        char appRootBuffer[AZ_MAX_PATH_LEN] = { 0 };

        ASSERT_TRUE(app.GetOptionalAppRootArg(appRootBuffer, AZ_ARRAY_SIZE(appRootBuffer)));
        ASSERT_STREQ(appRootBuffer, expectedResult);
    }

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppNoQuotedAppRoot_Success)
    {
    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* appRootArg = R"str(-approot=C:\path\to\app\root\)str";
        const char* expectedResult = R"str(C:\path\to\app\root\)str";
    #else
        const char* appRootArg = R"str(-approot=/path/to/app/root)str";
        const char* expectedResult = R"str(/path/to/app/root/)str";
    #endif

        const char* argArray[] = {
            appRootArg
        };
        int argc = AZ_ARRAY_SIZE(argArray);
        char** argv = const_cast<char**>(argArray); // this is unfortunately necessary to get around osx's strict non-const string literal stance

        AssetBuilderApplication app(&argc, &argv);

        char appRootBuffer[AZ_MAX_PATH_LEN] = { 0 };

        ASSERT_TRUE(app.GetOptionalAppRootArg(appRootBuffer, AZ_ARRAY_SIZE(appRootBuffer)));
        ASSERT_STREQ(appRootBuffer, expectedResult);
    }
    
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
        : ScopedAllocatorSetupFixture
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
