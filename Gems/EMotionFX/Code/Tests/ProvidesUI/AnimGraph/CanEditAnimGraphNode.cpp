/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <QApplication>
#include <QAction>
#include <QtTest>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include "AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx"
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <Source/Editor/PropertyWidgets/AnimGraphNodeNameHandler.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <Source/Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace EMotionFX
{
    TEST_F(UIFixture, CanEditAnimGraphNode)
    {
        /*
        Test Rail ID: C22083483
        Expected Result: Making changes to an AnimGraph Node through the Attributes Panel should reflect in the UI
        
        */
        RecordProperty("test_case_id", "C22083483");

        // Declare useful objects from other namespaces
        using AzToolsFramework::PropertyRowWidget;
        using AzToolsFramework::ReflectedPropertyEditor;
        using EMotionFX::AnimGraphNodeNameLineEdit;
        using AzToolsFramework::InstanceDataNode;

        // Typedefs to help minimize long defintions
        typedef AZStd::pair<InstanceDataNode*, PropertyRowWidget*> WidgetListItem;
        typedef AZStd::unordered_map<InstanceDataNode*, PropertyRowWidget*> WidgetList;
        
        // Constants
        const AZStd::string originalNodeName = "Original-Node";
        const AZStd::string motionNodeID = azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>();
        const QString newNodeName = "New-Node";

        // Load AnimGraph mode and get useful widgets/objects
        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");
        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();
        ASSERT_TRUE(graphWidget) << "Failed retrieving BlendGraphWidget";
        EMStudio::AttributesWindow* attributesWindow = animGraphPlugin->GetAttributesWindow();
        ASSERT_TRUE(attributesWindow) << "Failed retrieving Attributes Window";

        // Handle for active AnimGraph
        AnimGraph* activeAnimGraph = nullptr;
        {
            const AZStd::string createAnimGraphCommand = "CreateAnimGraph";
            const AZStd::string createNodeCommand = "AnimGraphCreateNode -type " + motionNodeID + " -name " + originalNodeName + " -parentName Root -xPos 1 -yPos 1";
            AZStd::string result;

            // Create a new AnimGraph
            EXPECT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "Expected there to be no anim graph loaded";
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(createAnimGraphCommand, result)) << result.c_str();
            activeAnimGraph = animGraphPlugin->GetActiveAnimGraph();
            ASSERT_TRUE(activeAnimGraph) << "An anim graph was not created with command: " << createAnimGraphCommand.c_str();

            // Create a new AnimGraph Node
            const size_t nodeCount = activeAnimGraph->GetNumNodes();
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(createNodeCommand, result)) << result.c_str();
            EXPECT_EQ(activeAnimGraph->GetNumNodes(), nodeCount + 1) << "Expected one more anim graph node after running command: " << createNodeCommand.c_str();
        }

        // Ensure the name of the created node is as expected
        AnimGraphNode* node = activeAnimGraph->GetNode(activeAnimGraph->GetNumNodes() - 1);
        ASSERT_TRUE(node) << "Failed retrieving node from active animgraph";
        EXPECT_EQ(originalNodeName, node->GetName()) << "Expected the created node to have the name: " << originalNodeName.c_str();

        // Select the new Graph Node (via left mouse click)
        graphWidget->resize(200, 200);
        EXPECT_EQ(graphWidget->CalcNumSelectedNodes(), 0) << "Expected to have exactly zero (0) selected nodes";
        QRect nodeRect = graphWidget->GetActiveGraph()->FindGraphNode(node)->GetFinalRect();
        QTest::mouseClick(graphWidget, Qt::LeftButton, Qt::KeyboardModifiers(), nodeRect.center());
        EXPECT_EQ(graphWidget->CalcNumSelectedNodes(), 1) << "Expected to have exactly one (1) selected node";
        
        auto objectEditor = attributesWindow->findChild<ObjectEditor*>("EMFX.AttributesWindow.ObjectEditor");
        auto propertyEditor = objectEditor->findChild<ReflectedPropertyEditor*>("PropertyEditor");

        // Get list of all PropertyRowWidgets (and their InstanceDataNodes)
        const WidgetList list = propertyEditor->GetWidgets();
        ASSERT_GT(list.size(), 0) << "Did not find any PropertyRowWidgets";

        // Look for PropertyRowWidget for "Name"
        PropertyRowWidget* propertyRow = nullptr;
        for (const WidgetListItem& item : list)
        {
            if (item.second->objectName() == "Name")
            {
                propertyRow = item.second;
            }
        }
        ASSERT_TRUE(propertyRow) << "Could not find the 'Name' PropertyRowWidget";

        // Set the text for the textbox to edit AnimGraph Node name
        auto lineEdit = static_cast<AnimGraphNodeNameLineEdit*>(propertyRow->GetChildWidget());
        lineEdit->setText(newNodeName);
        // Push changes in text to the AnimGraph Node
        lineEdit->OnEditingFinished();

        // Validate the name for the Node has changed
        EXPECT_EQ(newNodeName, node->GetName()) << "Expected the created node to have the name: " << newNodeName.toStdString();

        // Unselect AnimGraphNode before cleanup
        QPoint pt = QPoint(nodeRect.left() - 2, nodeRect.top() - 2); // Some point to click NOT in the AnimGraphNode's rectangle
        QTest::mouseClick(graphWidget, Qt::LeftButton, Qt::KeyboardModifiers(), pt);
        EXPECT_EQ(graphWidget->CalcNumSelectedNodes(), 0) << "Expected to have exactly zero (0) selected nodes";
    } // CanEditAnimGraph
} // EMotionFX namespace
