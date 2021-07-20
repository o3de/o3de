/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

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
        m_localFileIO->SetAlias("@engroot@", "dummy engine root");
    }

    void TearDown() override
    {
        AWSCoreFixture::TearDown();
        AWSCoreEditorUIFixture::TearDown();
    }
};

TEST_F(AWSCoreResourceMappingToolActionTest, AWSCoreResourceMappingToolAction_NoEngineRootFolder_ExpectOneError)
{
    m_localFileIO->ClearAlias("@engroot@");
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCoreResourceMappingToolAction testAction("dummy title");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error
}

TEST_F(AWSCoreResourceMappingToolActionTest, AWSCoreResourceMappingToolAction_UnableToFindExpectedFileOrFolder_ExpectFiveErrorsAndEmptyResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCoreResourceMappingToolAction testAction("dummy title");
    AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    EXPECT_TRUE(testAction.GetToolLaunchCommand() == "");
    EXPECT_TRUE(testAction.GetToolLogPath() == "");
    EXPECT_TRUE(testAction.GetToolReadMePath() == "");
}
