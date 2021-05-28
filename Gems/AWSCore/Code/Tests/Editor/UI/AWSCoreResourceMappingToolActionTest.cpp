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
