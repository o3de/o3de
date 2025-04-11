/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QAction>
#include <QtTest>
#include <qtoolbar.h>
#include <QWidget>
#include <QComboBox>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <EMotionFX/Source/MotionManager.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanNotActiveEmptyGraph)
    {
        // This test checks that activating an empty anim graph does nothing.
        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated.";
        ASSERT_EQ(0, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 0 anim graphs.";

        auto toolBar = animGraphPlugin->GetViewWidget()->findChild<QToolBar*>("EMFX.BlendGraphViewWidget.TopToolBar");
        QWidget* activateButton = UIFixture::GetWidgetFromToolbar(toolBar, "Activate Animgraph/State");

        ASSERT_TRUE(activateButton) << "Activate anim graph button not found.";

        QTest::mouseClick(activateButton, Qt::LeftButton);
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated after click the activate button.";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    
    class PopulatedAnimGraphFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            AZStd::string commandResult;
            MCore::CommandGroup group;

            // Create empty anim graph, add a motion, entry, hub, and  blend tree node.
            group.AddCommandString(AZStd::string::format("CreateAnimGraph -animGraphID %d", m_animGraphId));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 100 -yPos 100 -name %s",
                m_animGraphId, azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str(), m_motionNodeName.c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 100 -name %s",
                m_animGraphId, azrtti_typeid<BlendTree>().ToString<AZStd::string>().c_str(), m_blendTreeName.c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 100 -name %s",
                m_animGraphId, azrtti_typeid<AnimGraphHubNode>().ToString<AZStd::string>().c_str(), m_hubNodeName.c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 100 -name %s",
                m_animGraphId, azrtti_typeid<AnimGraphEntryNode>().ToString<AZStd::string>().c_str(), m_entryNodeName.c_str()));

            // Create new motion set
            group.AddCommandString(AZStd::string::format("CreateMotionSet -name motionSet0 -setID %d", m_motionSetID));

            // Run Commands
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();

            // Create temp Actor
            AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
            m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 1, "tempActor");

            // Cache some local poitners.
            m_animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            ASSERT_NE(m_animGraphPlugin, nullptr) << "Anim graph plugin not found.";

            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find newly created anim graph.";
        }

        void TearDown() override
        {
            GetEMotionFX().GetActorManager()->UnregisterAllActors();
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            m_actorAsset.Reset();
            UIFixture::TearDown();
        }

    public:
        const AZ::u32 m_animGraphId = 64;
        const AZ::u32 m_motionSetID = 32;
        AZStd::string m_motionNodeName = "testMotion";
        AZStd::string m_blendTreeName = "testBlendTree";
        AZStd::string m_hubNodeName = "testHub";
        AZStd::string m_entryNodeName = "testEntry";
        AnimGraph* m_animGraph = nullptr;
        EMStudio::AnimGraphPlugin* m_animGraphPlugin = nullptr;
        AZ::Data::Asset<Integration::ActorAsset> m_actorAsset;
    };
    
    TEST_F(PopulatedAnimGraphFixture, CanActivateValidGraph)
    {
        // This test checks that activating a filled anim graph will not crash and create an animgraph instance
        RecordProperty("test_case_id", "C1559131");

        // Find QComboBox that indicates preview of motionSet
        QComboBox* motionSetPreviewSelector = qobject_cast<QComboBox*>(PopulatedAnimGraphFixture::FindTopLevelWidget("EMFX.AttributesWindowWidget.AnimGraph.MotionSetComboBox"));

        // Set Preview motionset as created MotionSet
        motionSetPreviewSelector->setCurrentIndex(1);
        ASSERT_EQ(motionSetPreviewSelector->currentText(), "motionSet0") << "Preview Motionset could not be set";

        // Find activate animigraph button
        auto toolBar = m_animGraphPlugin->GetViewWidget()->findChild<QToolBar*>("EMFX.BlendGraphViewWidget.TopToolBar");
        QWidget* activateButton = PopulatedAnimGraphFixture::GetWidgetFromToolbar(toolBar, "Activate Animgraph/State");

        // Click activate animigraph button
        ASSERT_TRUE(activateButton) << "Activate anim graph button not found.";
        QTest::mouseClick(activateButton, Qt::LeftButton);

        // Confirm that the animigraph instance was created and is active
        ASSERT_TRUE(m_animGraphPlugin->GetActiveAnimGraph()) << "Anim graph should be activated.";
        ASSERT_EQ(1, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 1 anim graphs.";
    }
} // namespace EMotionFX
