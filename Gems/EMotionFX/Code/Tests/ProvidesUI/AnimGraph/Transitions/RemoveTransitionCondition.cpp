/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <QApplication>
#include <QtTest>
#include "qtestsystem.h"
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <Source/Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

namespace EMotionFX
{
    class RemoveTransitionConditionFixture : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();
            AZStd::string commandResult;

            MCore::CommandGroup group;

            // Create empty anim graph, add a motion node and a blend tree.
            group.AddCommandString(AZStd::string::format("CreateAnimGraph -animGraphID %d", m_animGraphId));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 100 -yPos 100 -name testMotion",
                m_animGraphId, azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 100 -name testBlendTree",
                m_animGraphId, azrtti_typeid<BlendTree>().ToString<AZStd::string>().c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateConnection -animGraphID %d -transitionType %s -sourceNode testMotion -targetNode testBlendTree",
                m_animGraphId, azrtti_typeid<AnimGraphStateTransition>().ToString<AZStd::string>().c_str()));

            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find newly created anim graph.";
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            UIFixture::TearDown();
        }

    public:
        const AZ::u32           m_animGraphId = 64;
        AnimGraph*              m_animGraph = nullptr;
    };

    TEST_F(RemoveTransitionConditionFixture, RemoveTransitionConditions)
    {
        RecordProperty("test_case_id", "C15031141");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";

        EMStudio::AnimGraphModel& animGraphModel = animGraphPlugin->GetAnimGraphModel();

        // Find the transition between the motion node and the blend tree.
        AnimGraphStateTransition* transition = m_animGraph->GetRootStateMachine()->GetTransition(0);
        ASSERT_TRUE(transition) << "Anim graph transition not found.";

        // Select the transition in the anim graph model.
        const QModelIndex& modelIndex = animGraphModel.FindFirstModelIndex(transition);
        ASSERT_TRUE(modelIndex.isValid()) << "Anim graph transition has an invalid model index.";
        animGraphModel.GetSelectionModel().select(QItemSelection(modelIndex, modelIndex), QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        // Ensure Theere is no initial condition
        ASSERT_EQ(0, transition->GetNumConditions()) << " Anim Graph Should not have a condition";

        // Create Condition
        // Find and "click" on Add Condition button
        EMStudio::AddConditionButton* addConditionButton = m_animGraphPlugin->GetAttributesWindow()->GetAddConditionButton();
        ASSERT_TRUE(addConditionButton) << "Add Condition button was not found";
        addConditionButton->OnCreateContextMenu();

        // Add a specific condition
        QAction* addPlayTimeConditionButton = RemoveTransitionConditionFixture::GetNamedAction(m_animGraphPlugin->GetAttributesWindow(), "Play Time Condition");
        ASSERT_TRUE(addPlayTimeConditionButton) << "'Play Time Condition' button was not found";
        addPlayTimeConditionButton->trigger();

        // Ensure Condition is created
        ASSERT_EQ(1, transition->GetNumConditions()) << " Anim Graph transition condition should have been made removed";

        // Find the created Condition
        auto objectEditor = animGraphPlugin->GetAttributesWindow()->findChild<QWidget*>("EMFX.AttributesWindowWidget.NodeTransition.ConditionsWidget");
        ASSERT_TRUE(objectEditor);
       
        const AzQtComponents::CardHeader* cardHeader = objectEditor->findChild<AzQtComponents::CardHeader*>();
        ASSERT_TRUE(cardHeader) << "Transition Condition CardHeader not found.";

        const QFrame* frame = cardHeader->findChild<QFrame*>("Background");
        ASSERT_TRUE(frame) << "Transtion Condition CardHeader Background Frame not found.";

        QPushButton* contextMenubutton = frame->findChild< QPushButton*>("ContextMenu");
        ASSERT_TRUE(contextMenubutton) << "Transition Condition ContextMenu not found.";

        // Pop up the context menu.
        QTest::mouseClick(contextMenubutton, Qt::LeftButton);
      
        QAction* deleteAction = RemoveTransitionConditionFixture::GetNamedAction(animGraphPlugin->GetAttributesWindow(), "Delete condition");
        ASSERT_TRUE(deleteAction) << "Delete Condition Action not found";
        deleteAction->trigger();

        // Check if the Condition get deleted.
        ASSERT_EQ(0, transition->GetNumConditions()) << " Anim Graph transition condition should be removed";
    }
} // namespace EMotionFX
