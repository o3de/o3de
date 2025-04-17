/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QtTest>
#include <QToolBar>
#include <QPushButton>

#include <AzToolsFramework/UI/PropertyEditor/PropertyCheckBoxCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    using TestParametersFixture = UIFixture;

    TEST_F(TestParametersFixture, CanChangeParameter)
    {
        // This test uses UIFixture to check that when parameters are added and edited in the Animgraph,
        // the results from the RPE are correctly updated in the Paramter itself.
        const AZ::u32 animGraphId = 64;
        AZStd::string commandResult;

        RecordProperty("test_case_id", "C5522322");

        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated.";
        ASSERT_EQ(EMotionFX::GetAnimGraphManager().GetNumAnimGraphs(), 0) << "Anim graph manager should contain 0 anim graph.";

        // Create empty anim graph and select it.
        const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
        AnimGraph* newGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
        EXPECT_NE(newGraph, nullptr) << "Cannot find newly created anim graph.";

        // The empty graph should contain one node (the root statemachine).
        ASSERT_TRUE(newGraph && newGraph->GetNumNodes() == 1) << "An empty anim graph should be activated.";
        ASSERT_EQ(EMotionFX::GetAnimGraphManager().GetNumAnimGraphs(), 1) << "Anim graph manager should contain 1 anim graph.";

        EMStudio::ParameterWindow* parameterWindow = animGraphPlugin->GetParameterWindow();
        ASSERT_TRUE(parameterWindow) << "Anim graph parameter window is invalid.";

        // Normally users press the + button and a context menu appears with the options to either add a parameter or a group.
        // We are bypassing the context menu and directly call the add parameter slot.
        parameterWindow->OnAddParameter();

        // Find parameter window.
        EMStudio::ParameterCreateEditWidget* parameterCreateWidget = qobject_cast<EMStudio::ParameterCreateEditWidget*>(FindTopLevelWidget("ParameterCreateEditWidget"));
        ASSERT_NE(parameterCreateWidget, nullptr) << "Cannot find anim graph parameter create/edit widget. Is the anim graph selected?";

        // Find the Create button
        QPushButton* createButton = parameterCreateWidget->findChild<QPushButton*>("EMFX.ParameterCreateEditWidget.CreateApplyButton");
        EXPECT_TRUE(createButton)<< "Cannot find Create Button";

        // Send the left button click directly to the button
        QTest::mouseClick(createButton, Qt::LeftButton);

        // Check we only have the one Parameter
        size_t numParameters = newGraph->GetNumParameters();
        EXPECT_EQ(numParameters, 1) << "Not just 1 parameter";

        const RangedValueParameter<float, FloatParameter>* parameter = reinterpret_cast<const RangedValueParameter<float, FloatParameter>* >(newGraph->FindValueParameter(0));
        EXPECT_TRUE(parameter) << "could not find the Parameter";

        // Find the parameter in the window
        QTreeWidget* treeWidget = parameterWindow->findChild<QTreeWidget*>("AnimGraphParamWindow");
        EXPECT_TRUE(treeWidget) << "Cannot find the QTreeWidget";

        // Select the node containing the parameter
        QTreeWidgetItem* parameterNodeItem = treeWidget->invisibleRootItem()->child(0);
        EXPECT_TRUE(parameterNodeItem) << "Parameter 0 not found";

        parameterNodeItem->setSelected(true);

        // Check that the values we are interested are set to their initial value
        EXPECT_TRUE(parameter->GetHasMinValue()) << "MinValue should be true";
        EXPECT_TRUE(parameter->GetHasMaxValue()) << "MaxValue should be true";

        // Find parameter widget.
        parameterCreateWidget = qobject_cast<EMStudio::ParameterCreateEditWidget*>(FindTopLevelWidget("ParameterCreateEditWidget"));
        ASSERT_NE(parameterCreateWidget, nullptr) << "Cannot find anim graph parameter create/edit widget. Is the anim graph selected?";
        
        // Get the parameter widget that holds ReflectedPropertyEditor
        AzToolsFramework::ReflectedPropertyEditor * parameterWidget = parameterCreateWidget->findChild<AzToolsFramework::ReflectedPropertyEditor*>("EMFX.ParameterCreateEditWidget.ReflectedPropertyEditor.ParameterEditorWidget");
        ASSERT_TRUE(parameterWidget) << "Cannot find the parameterWidget";

        AzToolsFramework::PropertyRowWidget* minimumWidget = reinterpret_cast<AzToolsFramework::PropertyRowWidget*>(GetNamedPropertyRowWidgetFromReflectedPropertyEditor(parameterWidget, "Has minimum"));
        EXPECT_TRUE(minimumWidget) << "Has minimum checkbox not found";

        AzToolsFramework::PropertyCheckBoxCtrl* checkBoxCtrl = reinterpret_cast< AzToolsFramework::PropertyCheckBoxCtrl* >(minimumWidget->GetChildWidget());
        EXPECT_TRUE(checkBoxCtrl) << "CheckBoxCtrl not found";
        QCheckBox* checkBox = checkBoxCtrl->GetCheckBox();
        EXPECT_TRUE(checkBox) << "CheckBox not found";
        checkBox->click();

        AzToolsFramework::PropertyRowWidget* maximumWidget = reinterpret_cast<AzToolsFramework::PropertyRowWidget*>(GetNamedPropertyRowWidgetFromReflectedPropertyEditor(parameterWidget, "Has maximum"));
        EXPECT_TRUE(maximumWidget) << "Has maximum checkbox not found";

        checkBoxCtrl = reinterpret_cast<AzToolsFramework::PropertyCheckBoxCtrl*>(maximumWidget->GetChildWidget());
        EXPECT_TRUE(checkBoxCtrl) << "CheckBoxCtrl not found";
        checkBox = checkBoxCtrl->GetCheckBox();
        EXPECT_TRUE(checkBox) << "CheckBox not found";
        checkBox->click();

        // Find the Apply button, until the changes are applied the values will not be updated
        QPushButton* applyButton = parameterCreateWidget->findChild<QPushButton*>("EMFX.ParameterCreateEditWidget.CreateApplyButton");
        EXPECT_TRUE(applyButton) << "Cannot find Apply Button";

        // Send the left button click directly to the button
        QTest::mouseClick(applyButton, Qt::LeftButton);

        EXPECT_FALSE(parameter->GetHasMinValue()) << "MinValue should be false";
        EXPECT_FALSE(parameter->GetHasMaxValue()) << "MaxValue should be false";
    }

    TEST_F(TestParametersFixture, CanRenameParameterGroup)
    {
        AZStd::string commandResult;
        RecordProperty("test_case_id", "C5522320");

        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        // Create anim graph.
        const AZ::u32 animGraphId = 64;
        AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
        AnimGraph* newGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
        EXPECT_NE(newGraph, nullptr) << "Cannot find newly created anim graph.";

        // Create parameter group.
        const AZStd::string initialGroupName = "Parameter Group 0";
        command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %d -name \"%s\"", animGraphId, initialGroupName.c_str());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
        EXPECT_EQ(newGraph->GetNumParameters(), 1) << "The newly created group should show up as a parameter.";
        const GroupParameter* groupParameter = azdynamic_cast<const GroupParameter*>(newGraph->FindParameter(0));
        EXPECT_TRUE(groupParameter) << "Cannot find the newly created parameter group.";

        // Get parameter window.
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_NE(animGraphPlugin, nullptr) << "Anim graph plugin not found.";
        EMStudio::ParameterWindow* parameterWindow = animGraphPlugin->GetParameterWindow();
        ASSERT_NE(parameterWindow, nullptr) << "Anim graph parameter window is not valid.";
        QTreeWidget* treeWidget = parameterWindow->findChild<QTreeWidget*>("AnimGraphParamWindow");
        ASSERT_NE(treeWidget, nullptr) << "Cannot find parameter tree widget.";

        // Select the parameter group.
        QTreeWidgetItem* parameterNodeItem = treeWidget->invisibleRootItem()->child(0);
        ASSERT_NE(parameterNodeItem, nullptr) << "Cannot select parameter group.";
        parameterNodeItem->setSelected(true);

        // Find edit parameter window and the line edit control for the name.
        EMStudio::ParameterCreateEditWidget* parameterCreateWidget = qobject_cast<EMStudio::ParameterCreateEditWidget*>(FindTopLevelWidget("ParameterCreateEditWidget"));
        ASSERT_NE(parameterCreateWidget, nullptr) << "Cannot find the edit parameter window.";
        AzToolsFramework::ReflectedPropertyEditor* parameterWidget = parameterCreateWidget->findChild<AzToolsFramework::ReflectedPropertyEditor*>("EMFX.ParameterCreateEditWidget.ReflectedPropertyEditor.ParameterEditorWidget");
        ASSERT_NE(parameterWidget, nullptr) << "Cannot find the reflected property editor.";
        AzToolsFramework::PropertyRowWidget* nameWidget = reinterpret_cast<AzToolsFramework::PropertyRowWidget*>(GetNamedPropertyRowWidgetFromReflectedPropertyEditor(parameterWidget, "Name"));
        ASSERT_NE(nameWidget, nullptr) << "Name widget not found";
        AzToolsFramework::PropertyStringLineEditCtrl* lineEditCtrl = reinterpret_cast< AzToolsFramework::PropertyStringLineEditCtrl* >(nameWidget->GetChildWidget());
        ASSERT_NE(lineEditCtrl, nullptr) << "Line edit control not found.";

        // Change the group name by editing the line edit.
        const AZStd::string changedGroupName = "Changed Group Name";
        lineEditCtrl->UpdateValue(changedGroupName.c_str());

        // Find and click the apply button.
        QPushButton* applyButton = parameterCreateWidget->findChild<QPushButton*>("EMFX.ParameterCreateEditWidget.CreateApplyButton");
        EXPECT_NE(applyButton, nullptr) << "Cannot find Apply button.";
        QTest::mouseClick(applyButton, Qt::LeftButton);

        // Data verification: Check if the group parameter name changed.
        EXPECT_STREQ(groupParameter->GetName().c_str(), changedGroupName.c_str());

        // Undo.
        EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(commandResult)) << commandResult.c_str();
        EXPECT_STREQ(groupParameter->GetName().c_str(), initialGroupName.c_str());

        // Redo.
        EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(commandResult)) << commandResult.c_str();
        EXPECT_STREQ(groupParameter->GetName().c_str(), changedGroupName.c_str());
    }
} // namespace EMotionFX
