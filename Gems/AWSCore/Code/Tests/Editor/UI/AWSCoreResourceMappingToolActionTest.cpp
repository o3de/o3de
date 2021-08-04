/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Editor/UI/AWSCoreResourceMappingToolAction.h>
#include <Editor/UI/AWSCoreEditorUIFixture.h>
#include <TestFramework/AWSCoreFixture.h>

#include <QAction>

using namespace AWSCore;

class AWSCoreResourceMappingToolActionTest
    : public AWSCoreFixture
    , public AWSCoreEditorUIFixture
{
    void SetUp() override
    {
        AWSCoreEditorUIFixture::SetUp();
        AWSCoreFixture::SetUp();
    }

    void TearDown() override
    {
        AWSCoreFixture::TearDown();
        AWSCoreEditorUIFixture::TearDown();
    }
};

TEST_F(AWSCoreResourceMappingToolActionTest, AWSCoreResourceMappingToolAction_NoEngineRootPath_ExpectErrorsAndResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCoreResourceMappingToolAction testAction("dummy title");
    EXPECT_TRUE(testAction.GetToolLaunchCommand() == "");
    AZ_TEST_STOP_TRACE_SUPPRESSION(4);
    AZStd::string expectedLogPath = AZStd::string::format("/%s/resource_mapping_tool.log", AWSCoreResourceMappingToolAction::ResourceMappingToolLogDirectoryPath);
    AzFramework::StringFunc::Path::Normalize(expectedLogPath);
    EXPECT_TRUE(testAction.GetToolLogFilePath() == expectedLogPath);
    EXPECT_TRUE(testAction.GetToolReadMePath() == "");
}
