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

#include <AzCore/Math/Crc.h>

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    class NodeUtils
    {
    public:
        static NodeTypeIdentifier ConstructNodeType(const Node* scriptCanvasNode);
        
        // Individual Identifier Methods
        static NodeTypeIdentifier ConstructEBusIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier);
        static NodeTypeIdentifier ConstructEBusEventSenderIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructEBusEventReceiverIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId);
        
        static NodeTypeIdentifier ConstructFunctionNodeIdentifier(const AZ::Data::AssetId& assetId);

        static NodeTypeIdentifier ConstructScriptEventIdentifier(const ScriptCanvas::EBusBusId& busId);
        static NodeTypeIdentifier ConstructSendScriptEventIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId);
        static NodeTypeIdentifier ConstructScriptEventReceiverIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId);
        
        static NodeTypeIdentifier ConstructCustomNodeIdentifier(const AZ::Uuid& nodeId);
        
        static NodeTypeIdentifier ConstructMethodNodeIdentifier(AZStd::string_view methodClass, AZStd::string_view methodName);
        
        static NodeTypeIdentifier ConstructGetVariableNodeIdentifier(const VariableId& variableId);
        static NodeTypeIdentifier ConstructSetVariableNodeIdentifier(const VariableId& variableId);
    };
}