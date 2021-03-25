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

#include <AWSCoreBus.h>
#include <AWSCoreEditor_Traits_Platform.h>
#include <Editor/UI/AWSCoreEditorMenu.h>
#include <Editor/UI/AWSCoreEditorUIFixture.h>
#include <TestFramework/AWSCoreFixture.h>

#include <QAction>
#include <QList>
#include <QString>

using namespace AWSCore;

static constexpr const int ExpectedActionNumOnWindowsPlatform = 8;
static constexpr const int ExpectedActionNumOnOtherPlatform = 6;

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

TEST_F(AWSCoreEditorMenuTest, AWSCoreEditorMenu_NoEngineRootFolder_ExpectOneError)
{
    m_localFileIO->ClearAlias("@engroot@");
    AZ_TEST_START_TRACE_SUPPRESSION;
    AWSCoreEditorMenu testMenu("dummy title");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error
}

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

    QList<QAction*> actualActions = testMenu.actions();
    for (QList<QAction*>::iterator itr = actualActions.begin(); itr != actualActions.end(); itr++)
    {
        if (QString::compare((*itr)->text(), AWSCoreEditorMenu::AWSClientAuthActionText) == 0)
        {
            EXPECT_TRUE((*itr)->isEnabled());
        }

        if (QString::compare((*itr)->text(), AWSCoreEditorMenu::AWSMetricsActionText) == 0)
        {
            EXPECT_TRUE((*itr)->isEnabled());
        }
    }
}
