/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QLineEdit>
#include <QMenu>
#include <QString>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ConnectionStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/CreateElementsStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ElementInteractionStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/VariableStates.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorautomationTest that creates a node from the node palette using drag/drop
    */
    class CreateNodeFromPaletteTest
        : public EditorAutomationTest
    {
    public:

        CreateNodeFromPaletteTest(const AZStd::string& nodeName, GraphCanvas::NodePaletteWidget* paletteWidget)
            : EditorAutomationTest(AZStd::string::format("Create %s from Node Palette", nodeName.c_str()).c_str())
        {
            AutomationStateModelId viewCenterModelId = "ViewCenter";

            AddState(new CreateRuntimeGraphState());
            AddState(new FindViewCenterState(viewCenterModelId));
            AddState(new CreateNodeFromPaletteState(paletteWidget, nodeName.c_str(), CreateNodeFromPaletteState::CreationType::ScenePosition, viewCenterModelId));
            AddState(new ForceCloseActiveGraphState());
        }
    };

    /**
        EditorautomationTest that creates a node from the context menu
    */
    class CreateNodeFromContextMenuTest
        : public EditorAutomationTest
    {
    public:

        CreateNodeFromContextMenuTest(const AZStd::string& nodeName)
            : EditorAutomationTest(AZStd::string::format("Create %s from Context Menu", nodeName.c_str()).c_str())
        {
            AutomationStateModelId viewCenterModelId = "ViewCenter";

            AddState(new CreateRuntimeGraphState());
            AddState(new FindViewCenterState(viewCenterModelId));
            AddState(new CreateNodeFromContextMenuState(nodeName.c_str(), CreateNodeFromContextMenuState::CreationType::ScenePosition, viewCenterModelId));
            AddState(new ForceCloseActiveGraphState());
        }
    };

    /**
        EditorautomationTest that creates a simple graph from the node palette and coupling.
    */
    class CreateHelloWorldFromPalette
        : public EditorAutomationTest
    {
    public:

        CreateHelloWorldFromPalette(GraphCanvas::NodePaletteWidget* paletteWidget)
            : EditorAutomationTest("Create Hello World From Palette")
        {
            AddState(new CreateRuntimeGraphState());

            AutomationStateModelId onGraphStartTargetPointId = "OnGraphStartScenePoint";
            AutomationStateModelId onGraphStartId = "OnGraphStartId";

            AddState(new FindViewCenterState(onGraphStartTargetPointId));
            AddState(new CreateNodeFromPaletteState(paletteWidget, "On Graph Start", CreateNodeFromPaletteState::CreationType::ScenePosition, onGraphStartTargetPointId, onGraphStartId));

            AutomationStateModelId printTargetPoint = "PrintScenePoint";
            AutomationStateModelId printId = "PrintId";

            FindPositionOffsets offsets;
            offsets.m_horizontalPosition = 1;
            offsets.m_horizontalOffset = 50;

            AddState(new FindNodePosition(onGraphStartId, printTargetPoint, offsets));
            AddState(new CreateNodeFromPaletteState(paletteWidget, "Print", CreateNodeFromPaletteState::CreationType::ScenePosition, printTargetPoint, printId));

            AddState(new CoupleNodesState(onGraphStartId, GraphCanvas::ConnectionType::CT_Output, printId));
            AddState(new ForceCloseActiveGraphState());
        }
    };

    /**
        EditorautomationTest that creates a simple graph from the context menu and coupling.
    */
    class CreateHelloWorldFromContextMenu
        : public EditorAutomationTest
    {
    public:

        CreateHelloWorldFromContextMenu()
            : EditorAutomationTest("Create Hello World From Context Menu")
        {
            AddState(new CreateRuntimeGraphState());

            AutomationStateModelId onGraphStartTargetPointId = "OnGraphStartScenePoint";
            AutomationStateModelId onGraphStartId = "OnGraphStartId";

            AddState(new FindViewCenterState(onGraphStartTargetPointId));
            AddState(new CreateNodeFromContextMenuState("On Graph Start", CreateNodeFromContextMenuState::CreationType::ScenePosition, onGraphStartTargetPointId, onGraphStartId));

            AutomationStateModelId printTargetPoint = "PrintScenePoint";
            AutomationStateModelId printId = "PrintId";

            FindPositionOffsets offsets;
            offsets.m_horizontalPosition = 1;
            offsets.m_horizontalOffset = 50;

            AddState(new FindNodePosition(onGraphStartId, printTargetPoint, offsets));
            AddState(new CreateNodeFromContextMenuState("Print", CreateNodeFromContextMenuState::CreationType::ScenePosition, printTargetPoint, printId));

            AddState(new CoupleNodesState(onGraphStartId, GraphCanvas::ConnectionType::CT_Output, printId));
            AddState(new ForceCloseActiveGraphState());
        }
    };

    /**
        EditorautomationTest that creates all of the nodes under the specified category
    */
    class CreateCategoryTest
        : public EditorAutomationTest
    {
    public:
        CreateCategoryTest(AZStd::string categoryString, GraphCanvas::NodePaletteWidget* nodePaletteWidget);
        ~CreateCategoryTest() override = default;
    };

    /**
        EditorautomationTest that will splice the specified node onto a simple graph using execution connections and the context menu
    */
    class CreateExecutionSplicedNodeTest
        : public EditorAutomationTest
    {
    public:
        CreateExecutionSplicedNodeTest(QString nodeName);
        ~CreateExecutionSplicedNodeTest() override = default;
    };

    /**
        EditorautomationTest that will splice the specified node onto a simple graph using execution connections and dragging/dropping
    */
    class CreateDragDropExecutionSpliceNodeTest
        : public EditorAutomationTest
    {
    public:
        CreateDragDropExecutionSpliceNodeTest(GraphCanvas::NodePaletteWidget* nodePaletteWidget, QString nodeName);
        ~CreateDragDropExecutionSpliceNodeTest() override = default;
    };
}
