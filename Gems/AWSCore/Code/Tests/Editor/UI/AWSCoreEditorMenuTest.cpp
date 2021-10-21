/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AWSCoreBus.h>
#include <AWSCoreEditor_Traits_Platform.h>
#include <Editor/Constants/AWSCoreEditorMenuNames.h>
#include <Editor/UI/AWSCoreEditorMenu.h>
#include <Editor/UI/AWSCoreEditorUIFixture.h>
#include <TestFramework/AWSCoreFixture.h>

#include <QAction>
#include <QList>
#include <QString>

using namespace AWSCore;

static constexpr const int ExpectedActionNumOnWindowsPlatform = 9;
static constexpr const int ExpectedActionNumOnOtherPlatform = 7;

class AWSCoreEditorMenuTest
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

TEST_F(AWSCoreEditorMenuTest, AWSCoreEditorMenu_GetAllActions_GetExpectedNumberOfActions)
{
    AWSCoreEditorMenu testMenu("dummy title");

    QList<QAction*> actualActions = testMenu.actions();
#ifdef AWSCORE_EDITOR_RESOURCE_MAPPING_TOOL_ENABLED
    EXPECT_TRUE(actualActions.size() == ExpectedActionNumOnWindowsPlatform);
#else
    EXPECT_TRUE(actualActions.size() == ExpectedActionNumOnOtherPlatform);
#endif
}

TEST_F(AWSCoreEditorMenuTest, AWSCoreEditorMenu_BroadcastFeatureGemsAreEnabled_CorrespondingActionsAreEnabled)
{
    AWSCoreEditorMenu testMenu("dummy title");

    AWSCoreEditorRequestBus::Broadcast(&AWSCoreEditorRequests::SetAWSClientAuthEnabled);
    AWSCoreEditorRequestBus::Broadcast(&AWSCoreEditorRequests::SetAWSMetricsEnabled);
    AWSCoreEditorRequestBus::Broadcast(&AWSCoreEditorRequests::SetAWSGameLiftEnabled);

    QList<QAction*> actualActions = testMenu.actions();
    for (QList<QAction*>::iterator itr = actualActions.begin(); itr != actualActions.end(); itr++)
    {
        if (QString::compare((*itr)->text(), AWSClientAuthActionText) == 0)
        {
            EXPECT_TRUE((*itr)->isEnabled());
        }

        if (QString::compare((*itr)->text(), AWSMetricsActionText) == 0)
        {
            EXPECT_TRUE((*itr)->isEnabled());
        }

        if (QString::compare((*itr)->text(), AWSGameLiftActionText) == 0)
        {
            EXPECT_TRUE((*itr)->isEnabled());
        }
    }
}
