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

class DISABLED_AWSCoreResourceMappingToolActionTest
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

TEST_F(DISABLED_AWSCoreResourceMappingToolActionTest, AWSCoreResourceMappingToolAction_NoEngineRootPath_ExpectErrorsAndResult)
{
    AWSCoreResourceMappingToolAction testAction("dummy title");
    AZ_TEST_START_TRACE_SUPPRESSION;
    EXPECT_EQ("", testAction.GetToolLaunchCommand());
    EXPECT_EQ("", testAction.GetToolLogFilePath());
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);
}
