/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <QTableWidget>
#include <gtest/gtest.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/Matchers.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#include <QApplication>
#include <QComboBox>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <Editor/ReselectingTreeView.h>
#include <QtTest>
#include <QApplication>
#include <QWidget>
#include <QToolBar>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>





namespace EMotionFX
{
    class EditGroupFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            // Create empty anim graph and select it.
            const AZ::u32 animGraphId = 1;
            const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId);
            AZStd::string commandResult;

            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find the newly created anim graph.";
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            UIFixture::TearDown();
        }

    public:
        AnimGraph* m_animGraph = nullptr;
    };

    TEST_F(EditGroupFixture, CanEditParameterGroup)
    {
        RecordProperty("test_case_id", "C5522320");
        using WidgetMap = AZStd::unordered_map<AzToolsFramework::InstanceDataNode*, AzToolsFramework::PropertyRowWidget*>;

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";

        // Verify baseline
        ASSERT_EQ(m_animGraph->GetNumParameters(), 0) << "No Parameter Groups should exist initially";
        EXPECT_EQ(m_animGraph->GetNumValueParameters(), 0) << "No parameters should exist initially";

        AZStd::string result;

        // Create Parameter Group
        AZStd::string command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\"", m_animGraph->GetID(), "Group0");
        ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result)) << "Parameter Group could not be created" << result.c_str();

        // Verify Parameter Group was created
        // Verify that we have the group inside the anim graph.
        ASSERT_EQ(m_animGraph->GetNumParameters(), 1) << "Group creation failed. We shouldn't have one parameter in the animgraph.";
        EXPECT_EQ(m_animGraph->GetNumValueParameters(), 0) << "Expecting no value parameters as we only created a group.";
        const Parameter* parameter = m_animGraph->FindParameter(0);
        EXPECT_NE(parameter, nullptr) << "The parameter we created should be valid.";
        ASSERT_THAT(parameter->GetName(), StrEq("Group0"));

        // Edit name of Parameter group
        EMStudio::ParameterWindow* parameterWindow = animGraphPlugin->GetParameterWindow();
        ASSERT_TRUE(parameterWindow) << "Anim graph parameter window is invalid.";

        parameterWindow->SelectParameters({ "Group0" });
        parameterWindow->OnEditSelected();

        auto groupEditWidget = static_cast<EMStudio::ParameterCreateEditWidget*>(FindTopLevelWidget("ParameterCreateEditWidget"));
        ASSERT_NE(groupEditWidget, nullptr) << "Cannot find anim graph group edit dialog";
        auto propertyEditor = groupEditWidget->findChild<AzToolsFramework::ReflectedPropertyEditor*>("EMFX.ParameterCreateEditWidget.ReflectedPropertyEditor.ParameterEditorWidget");

        // Get list of all PropertyRowWidgets (and their InstanceDataNodes)
        const WidgetMap& list = propertyEditor->GetWidgets();
        ASSERT_GT(list.size(), 0) << "Did not find any PropertyRowWidgets";

        // Look for PropertyRowWidget for "Name"

        const auto propertyRowIt = AZStd::find_if(list.begin(), list.end(), [](const WidgetMap::value_type& item) {
            return item.second->objectName() == "Name";
        });
        ASSERT_NE(propertyRowIt, list.end());
        const AzToolsFramework::PropertyRowWidget* propertyRow = propertyRowIt->second;

        // Set the text for the textbox to edit AnimGraph Node name
        auto lineEdit = static_cast<AzToolsFramework::PropertyStringLineEditCtrl*>(propertyRow->GetChildWidget());
        QString newName = "DiffGroup";
        lineEdit->UpdateValue(newName);
        groupEditWidget->accept();

        // Verify that name was changed
        ASSERT_THAT(parameter->GetName(), StrEq("DiffGroup"));
    }
} // namespace EMotionFX
