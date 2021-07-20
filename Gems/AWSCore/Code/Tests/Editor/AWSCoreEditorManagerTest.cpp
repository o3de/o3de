/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Editor/AWSCoreEditorManager.h>
#include <Editor/UI/AWSCoreEditorMenu.h>
#include <Editor/UI/AWSCoreEditorUIFixture.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSCoreEditorManagerTest
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

TEST_F(AWSCoreEditorManagerTest, AWSCoreEditorManager_Constructor_HaveExpectedUIResourcesCreated)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCoreEditorManager testManager;
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error
    EXPECT_TRUE(testManager.GetAWSCoreEditorMenu());
}
