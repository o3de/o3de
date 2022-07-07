/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    class NodeUtils
    {
    public:
        static NodeTypeIdentifier ConstructNodeType(const Node* scriptCanvasNode);
        
        // Individual Identifier Methods
        static NodeTypeIdentifier ConstructEBusIdentifier(ScriptCanvas::EBusBusId ebusIdentifier);
        static NodeTypeIdentifier ConstructEBusEventSenderIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructEBusEventSenderOverloadedIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructEBusEventReceiverIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId);
        // \todo examine if this is required
        // static NodeTypeIdentifier ConstructEBusEventReceiverOverloadedIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructFunctionNodeIdentifier(const AZ::Data::AssetId& assetId);
        static NodeTypeIdentifier ConstructEmbeddedFunctionNodeIdentifier(const AZ::Data::AssetId& assetId);

        static NodeTypeIdentifier ConstructScriptEventIdentifier(ScriptCanvas::EBusBusId busId);
        static NodeTypeIdentifier ConstructSendScriptEventIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructScriptEventReceiverIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId);
        
        static NodeTypeIdentifier ConstructCustomNodeIdentifier(const AZ::Uuid& nodeId);
        
        static NodeTypeIdentifier ConstructMethodNodeIdentifier(AZStd::string_view methodClass, AZStd::string_view methodName, ScriptCanvas::PropertyStatus propertyStatus);
        static NodeTypeIdentifier ConstructGlobalMethodNodeIdentifier(AZStd::string_view methodName);
        static NodeTypeIdentifier ConstructMethodOverloadedNodeIdentifier(AZStd::string_view methodName);

        static NodeTypeIdentifier ConstructGetVariableNodeIdentifier(const VariableId& variableId);
        static NodeTypeIdentifier ConstructSetVariableNodeIdentifier(const VariableId& variableId);
        static NodeTypeIdentifier ConstructAzEventIdentifier(AzEventIdentifier azEventIdentifier);

        static void InitializeNode(Node* node, const NodeReplacementConfiguration& config);
    };
}
