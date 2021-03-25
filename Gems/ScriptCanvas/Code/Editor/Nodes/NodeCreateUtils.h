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

#pragma once

#include <AzCore/Component/EntityId.h>

#include <Editor/Nodes/NodeUtils.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptEvents
{
    class Method;
}

namespace ScriptCanvasEditor::Nodes
{
    // Specific create methods which will also handle displaying the node.
    NodeIdPair CreateNode(const AZ::Uuid& classData, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const StyleConfiguration& styleConfiguration);
    NodeIdPair CreateEntityNode(const AZ::EntityId& sourceId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
    NodeIdPair CreateObjectMethodNode(AZStd::string_view className, AZStd::string_view methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
    NodeIdPair CreateObjectMethodOverloadNode(AZStd::string_view className, AZStd::string_view methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId);
    NodeIdPair CreateGlobalMethodNode(AZStd::string_view methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
    NodeIdPair CreateEbusWrapperNode(AZStd::string_view busName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);

    // Script Events
    NodeIdPair CreateScriptEventReceiverNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId);
    NodeIdPair CreateScriptEventSenderNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);

    NodeIdPair CreateGetVariableNode(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasGraphId);
    NodeIdPair CreateSetVariableNode(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasGraphId);

    // Functions
    NodeIdPair CreateFunctionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId, const AZ::Data::AssetId& assetId);

    // Nodelings
    NodeIdPair CreateExecutionNodeling(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, AZStd::string_view rootName = "New Nodeling");

    // AZ Event
    NodeIdPair CreateAzEventHandlerNode(const AZ::BehaviorMethod& methodWithAzEventReturn, ScriptCanvas::ScriptCanvasId scriptCanvasId,
        AZ::EntityId connectingMethodNodeId);
}
