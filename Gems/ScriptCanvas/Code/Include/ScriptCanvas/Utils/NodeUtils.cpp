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
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>

namespace ScriptCanvas
{
    //////////////
    // NodeUtils
    //////////////
    
    NodeTypeIdentifier NodeUtils::ConstructNodeType(const Node* scriptCanvasNode)
    {   
        if (auto sendScriptEventNode = azrtti_cast<const Nodes::Core::SendScriptEvent*>(scriptCanvasNode))
        {
            return ConstructSendScriptEventIdentifier(sendScriptEventNode->GetBusId(), sendScriptEventNode->GetEventId());
        }
        else if (auto receiveScriptEventNode = azrtti_cast<const Nodes::Core::ReceiveScriptEvent*>(scriptCanvasNode))
        {
            return ConstructScriptEventIdentifier(receiveScriptEventNode->GetBusId());
        }
        else if (auto methodNode = azrtti_cast<const Nodes::Core::Method*>(scriptCanvasNode))
        {
            if (methodNode->GetMethodType() == Nodes::Core::MethodType::Event)
            {
                // TODO: Make this use some proper IDs rather then regenerating them here.
                return ConstructEBusEventSenderIdentifier(ScriptCanvas::EBusEventId(methodNode->GetRawMethodClassName()), ScriptCanvas::EBusEventId(methodNode->GetName()));
            }
            else
            {
                return ConstructMethodNodeIdentifier(methodNode->GetRawMethodClassName(), methodNode->GetName());
            }
        }
        else if (auto ebusNode = azrtti_cast<const Nodes::Core::EBusEventHandler*>(scriptCanvasNode))
        {
            return ConstructEBusIdentifier(ebusNode->GetEBusId());
        }
        else if (auto getVariableNode = azrtti_cast<const ScriptCanvas::Nodes::Core::GetVariableNode*>(scriptCanvasNode))
        {
            return ConstructGetVariableNodeIdentifier(getVariableNode->GetId());
        }
        else if (auto setVariableNode = azrtti_cast<const ScriptCanvas::Nodes::Core::SetVariableNode*>(scriptCanvasNode))
        {
            return ConstructSetVariableNodeIdentifier(setVariableNode->GetId());
        }
        else if (auto functionNode = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionNode*>(scriptCanvasNode))
        {
            return ConstructFunctionNodeIdentifier(functionNode->GetAssetId());
        }
        else
        {
            return ConstructCustomNodeIdentifier(scriptCanvasNode->RTTI_GetType());
        }

        return NodeTypeIdentifier(0);
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusIdentifier(const AZ::Crc32& ebusIdentifier)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::EBusEventHandler>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Crc32>()(ebusIdentifier));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusEventSenderIdentifier(const AZ::Crc32& ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::Method>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Crc32>()(ebusIdentifier));
        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusEventReceiverIdentifier(const AZ::Crc32& ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = ConstructEBusIdentifier(ebusIdentifier);

        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructScriptEventIdentifier(const ScriptCanvas::EBusBusId& busId)
    {
        NodeTypeIdentifier resultHash = 0;

        // Just going to use the ReceiveNode to isolate this out. This can be used for both the send and the receiver as it
        // generically identifies the ScriptEvent.
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusBusId>()(busId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructSendScriptEventIdentifier(const ScriptCanvas::EBusBusId& ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::SendScriptEvent>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusBusId>()(ebusIdentifier));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructScriptEventReceiverIdentifier(const AZ::Crc32& ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = ConstructScriptEventIdentifier(ebusIdentifier);

        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructMethodNodeIdentifier(AZStd::string_view methodClass, AZStd::string_view methodName)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::Method>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZStd::string_view>()(methodClass));
        AZStd::hash_combine(resultHash, AZStd::hash<AZStd::string>()(methodName));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructCustomNodeIdentifier(const AZ::Uuid& nodeTypeId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(nodeTypeId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructGetVariableNodeIdentifier(const VariableId& variableId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::GetVariableNode>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::VariableId>()(variableId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructSetVariableNodeIdentifier(const VariableId& variableId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::SetVariableNode>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::VariableId>()(variableId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructFunctionNodeIdentifier(const AZ::Data::AssetId& assetId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::FunctionNode>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Data::AssetId>()(assetId));

        return resultHash;
    }
}