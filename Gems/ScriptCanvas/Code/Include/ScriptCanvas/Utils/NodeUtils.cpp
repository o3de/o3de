/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/MethodOverloaded.h>
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
            if (auto overloadNode = azrtti_cast<const Nodes::Core::MethodOverloaded*>(scriptCanvasNode))
            {
                if (overloadNode->GetMethodType() == MethodType::Event)
                {
                    // TODO: Make this use some proper IDs rather then regenerating them here.
                    return ConstructEBusEventSenderOverloadedIdentifier(ScriptCanvas::EBusEventId(overloadNode->GetRawMethodClassName()), ScriptCanvas::EBusEventId(overloadNode->GetName()));
                }
                else
                {
                    return ConstructMethodOverloadedNodeIdentifier(overloadNode->GetName());
                }
            }
            else if (methodNode->GetMethodType() == MethodType::Event)
            {
                // TODO: Make this use some proper IDs rather then regenerating them here.
                return ConstructEBusEventSenderIdentifier(ScriptCanvas::EBusEventId(methodNode->GetRawMethodClassName()), ScriptCanvas::EBusEventId(methodNode->GetName()));
            }
            else
            {
                return ConstructMethodNodeIdentifier(methodNode->GetRawMethodClassName(), methodNode->GetName(), methodNode->GetPropertyStatus());
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
        else if (auto functionNode = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(scriptCanvasNode))
        {
            return ConstructFunctionNodeIdentifier(functionNode->GetAssetId());
        }
        else
        {
            return ConstructCustomNodeIdentifier(scriptCanvasNode->RTTI_GetType());
        }
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusIdentifier(ScriptCanvas::EBusBusId ebusIdentifier)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::EBusEventHandler>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Crc32>()(ebusIdentifier));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusEventSenderIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::Method>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Crc32>()(ebusIdentifier));
        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusEventSenderOverloadedIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::MethodOverloaded>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Crc32>()(ebusIdentifier));
        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEBusEventReceiverIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = ConstructEBusIdentifier(ebusIdentifier);

        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructScriptEventIdentifier(ScriptCanvas::EBusBusId busId)
    {
        NodeTypeIdentifier resultHash = 0;

        // Just going to use the ReceiveNode to isolate this out. This can be used for both the send and the receiver as it
        // generically identifies the ScriptEvent.
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusBusId>()(busId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructSendScriptEventIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::SendScriptEvent>()));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusBusId>()(ebusIdentifier));
        AZStd::hash_combine(resultHash, AZStd::hash<ScriptCanvas::EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructScriptEventReceiverIdentifier(ScriptCanvas::EBusBusId ebusIdentifier, const EBusEventId& eventId)
    {
        NodeTypeIdentifier resultHash = ConstructScriptEventIdentifier(ebusIdentifier);

        AZStd::hash_combine(resultHash, AZStd::hash<EBusEventId>()(eventId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructMethodNodeIdentifier(AZStd::string_view methodClass, AZStd::string_view methodName, ScriptCanvas::PropertyStatus propertyStatus)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::Method>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZStd::string_view>()(methodClass));
        AZStd::hash_combine(resultHash, AZStd::hash<AZStd::string>()(methodName));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::u8>()(static_cast<AZ::u8>(propertyStatus)));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructGlobalMethodNodeIdentifier(AZStd::string_view methodName)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::Method>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZStd::string_view>()(methodName));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructMethodOverloadedNodeIdentifier(AZStd::string_view methodName)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::MethodOverloaded>()));
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

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::FunctionCallNode>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Data::AssetId>()(assetId.m_guid));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructEmbeddedFunctionNodeIdentifier(const AZ::Data::AssetId& assetId)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::FunctionCallNode>()));
        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Data::AssetId>()(assetId));

        return resultHash;
    }

    NodeTypeIdentifier NodeUtils::ConstructAzEventIdentifier(AzEventIdentifier azEventIdentifier)
    {
        NodeTypeIdentifier resultHash = 0;

        AZStd::hash_combine(resultHash, AZStd::hash<AZ::Uuid>()(azrtti_typeid<ScriptCanvas::Nodes::Core::AzEventHandler>()));
        AZStd::hash_combine(resultHash, AZStd::hash<size_t>()(static_cast<size_t>(azEventIdentifier)));

        return resultHash;
    }

    void NodeUtils::InitializeNode(Node* node, const NodeReplacementConfiguration& config)
    {
        if (auto* method = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node))
        {
            ScriptCanvas::NamespacePath emptyNamespaces;
            method->InitializeBehaviorMethod(emptyNamespaces, config.m_className, config.m_methodName, config.m_propertyStatus);
        }
    }
}
