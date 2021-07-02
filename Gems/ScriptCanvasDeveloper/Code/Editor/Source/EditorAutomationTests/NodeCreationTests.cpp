/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <qpushbutton.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

#include <EditorAutomationTests/NodeCreationTests.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

namespace ScriptCanvasDeveloper
{
    ///////////////////////
    // CreateCategoryTest
    ///////////////////////

    CreateCategoryTest::CreateCategoryTest(AZStd::string categoryString, GraphCanvas::NodePaletteWidget* nodePaletteWidget)
        : EditorAutomationTest(QString("Create %1 Category from Palette").arg(categoryString.c_str()))
    {
        AutomationStateModelId categoryId = "CategoryId";
        SetStateData(categoryId, categoryString);

        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId viewCenterId = "ViewCenterId";
        AddState(new FindViewCenterState(viewCenterId));

        AddState(new CreateCategoryFromNodePaletteState(nodePaletteWidget, categoryId, viewCenterId));

        AddState(new ForceCloseActiveGraphState());
    }

    ///////////////////////////////////
    // CreateExecutionSplicedNodeTest
    ///////////////////////////////////

    CreateExecutionSplicedNodeTest::CreateExecutionSplicedNodeTest(QString nodeName)
        : EditorAutomationTest(QString("Create %1 via Connection Splice").arg(nodeName))
    {
        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId onGraphStartTargetPointId = "OnGraphStartScenePoint";
        AutomationStateModelId onGraphStartId = "OnGraphStartId";

        AddState(new FindViewCenterState(onGraphStartTargetPointId));
        AddState(new CreateNodeFromContextMenuState("On Graph Start", CreateNodeFromContextMenuState::CreationType::ScenePosition, onGraphStartTargetPointId, onGraphStartId));

        AutomationStateModelId onGraphStartEndpoint = "OnGraphStart::ExecutionEndpoint";

        AddState(new FindEndpointOfTypeState(onGraphStartId, onGraphStartEndpoint, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot));

        AutomationStateModelId printNodeId = "PrintId";
        AutomationStateModelId spliceTargetId = "SpliceTargetId";
        AddState(new CreateNodeFromProposalState("Print", onGraphStartEndpoint, "", printNodeId, spliceTargetId));

        AddState(new CreateNodeFromContextMenuState(nodeName, CreateNodeFromContextMenuState::CreationType::Splice, spliceTargetId));

        AddState(new ForceCloseActiveGraphState());
    }

    ///////////////////////////////////
    // CreateExecutionSplicedNodeTest
    ///////////////////////////////////

    CreateDragDropExecutionSpliceNodeTest::CreateDragDropExecutionSpliceNodeTest(GraphCanvas::NodePaletteWidget* nodePaletteWidget, QString nodeName)
        : EditorAutomationTest(QString("Create %1 via Dropped Connection Splice").arg(nodeName))
    {
        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId onGraphStartTargetPointId = "OnGraphStartScenePoint";
        AutomationStateModelId onGraphStartId = "OnGraphStartId";

        AddState(new FindViewCenterState(onGraphStartTargetPointId));
        AddState(new CreateNodeFromContextMenuState("On Graph Start", CreateNodeFromContextMenuState::CreationType::ScenePosition, onGraphStartTargetPointId, onGraphStartId));

        AutomationStateModelId onGraphStartEndpoint = "OnGraphStart::ExecutionEndpoint";

        AddState(new FindEndpointOfTypeState(onGraphStartId, onGraphStartEndpoint, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot));

        AutomationStateModelId printNodeId = "PrintId";
        AutomationStateModelId spliceTargetId = "SpliceTargetId";
        AddState(new CreateNodeFromProposalState("Print", onGraphStartEndpoint, "", printNodeId, spliceTargetId));

        AddState(new CreateNodeFromPaletteState(nodePaletteWidget, nodeName, CreateNodeFromPaletteState::CreationType::Splice, spliceTargetId));

        AddState(new ForceCloseActiveGraphState());
    }
}
