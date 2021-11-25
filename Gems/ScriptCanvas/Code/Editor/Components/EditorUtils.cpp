/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <ScriptCanvas/Components/EditorUtils.h>
AZ_POP_DISABLE_WARNING

#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{
    AZStd::optional<SourceHandle> CompleteDescription(const SourceHandle& source)
    {
        if (source.IsDescriptionValid())
        {
            return source;
        }

        AzToolsFramework::AssetSystemRequestBus::Events* assetSystem = AzToolsFramework::AssetSystemRequestBus::FindFirstHandler();
        if (assetSystem)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;

            if (!source.Id().IsNull())
            {
                if (assetSystem->GetSourceInfoBySourceUUID(source.Id(), assetInfo, watchFolder))
                {
                    AZ::IO::Path watchPath(watchFolder);
                    AZ::IO::Path assetInfoPath(assetInfo.m_relativePath);
                    SourceHandle fullPathHandle(nullptr, assetInfo.m_assetId.m_guid, watchPath / assetInfoPath);

                    if (assetSystem->GetSourceInfoBySourcePath(fullPathHandle.Path().c_str(), assetInfo, watchFolder) && assetInfo.m_assetId.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", assetInfo.m_assetId.m_guid == source.Id(), "SourceHandle completion produced conflicting AssetId.");
                        auto path = fullPathHandle.Path();
                        return SourceHandle(source, assetInfo.m_assetId.m_guid, path.MakePreferred());
                    }
                }
            }

            if (!source.Path().empty())
            {
                if (assetSystem->GetSourceInfoBySourcePath(source.Path().c_str(), assetInfo, watchFolder) && assetInfo.m_assetId.IsValid())
                {
                    return SourceHandle(source, assetInfo.m_assetId.m_guid, source.Path());
                }
            }
        }


        return AZStd::nullopt;
    }

    bool CompleteDescriptionInPlace(SourceHandle& source)
    {
        if (auto completed = CompleteDescription(source))
        {
            source = *completed;
            return true;
        }
        else
        {
            return false;
        }
    }

    //////////////////////////
    // NodeIdentifierFactory
    //////////////////////////

    ScriptCanvas::NodeTypeIdentifier NodeIdentifierFactory::ConstructNodeIdentifier(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        ScriptCanvas::NodeTypeIdentifier resultHash = 0;
        if (auto getVariableMethodTreeItem = azrtti_cast<const GetVariableNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructGetVariableNodeIdentifier(getVariableMethodTreeItem->GetVariableId());
        }
        else if (auto setVariableMethodTreeItem = azrtti_cast<const SetVariableNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructSetVariableNodeIdentifier(setVariableMethodTreeItem->GetVariableId());
        }
        else if (auto classMethodTreeItem = azrtti_cast<const ClassMethodEventPaletteTreeItem*>(treeItem))
        {
            if (classMethodTreeItem->IsOverload())
            {
                resultHash = ScriptCanvas::NodeUtils::ConstructMethodOverloadedNodeIdentifier(classMethodTreeItem->GetMethodName());
            }
            else
            {
                resultHash = ScriptCanvas::NodeUtils::ConstructMethodNodeIdentifier(classMethodTreeItem->GetClassMethodName(), classMethodTreeItem->GetMethodName(), classMethodTreeItem->GetPropertyStatus());
            }
        }
        else if (auto globalMethodTreeItem = azrtti_cast<const GlobalMethodEventPaletteTreeItem*>(treeItem); globalMethodTreeItem != nullptr)
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructGlobalMethodNodeIdentifier(globalMethodTreeItem->GetMethodName());
        }
        else if (auto customNodeTreeItem = azrtti_cast<const CustomNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructCustomNodeIdentifier(customNodeTreeItem->GetTypeId());
        }
        else if (auto sendEbusEventTreeItem = azrtti_cast<const EBusSendEventPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructEBusEventSenderIdentifier(sendEbusEventTreeItem->GetBusId(), sendEbusEventTreeItem->GetEventId());
        }
        else if (auto handleEbusEventTreeItem = azrtti_cast<const EBusHandleEventPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(handleEbusEventTreeItem->GetBusId(), handleEbusEventTreeItem->GetEventId());
        }
        else if (auto functionTreeItem = azrtti_cast<const FunctionPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructFunctionNodeIdentifier(functionTreeItem->GetAssetId());
        }

        return resultHash;
    }

    AZStd::vector< ScriptCanvas::NodeTypeIdentifier > NodeIdentifierFactory::ConstructNodeIdentifiers(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        AZStd::vector< ScriptCanvas::NodeTypeIdentifier > nodeIdentifiers;

        if (auto scriptEventTreeItem = azrtti_cast<const ScriptEventsEventNodePaletteTreeItem*>(treeItem))
        {
            nodeIdentifiers.emplace_back(ScriptCanvas::NodeUtils::ConstructScriptEventReceiverIdentifier(scriptEventTreeItem->GetBusIdentifier(), scriptEventTreeItem->GetEventIdentifier()));
            nodeIdentifiers.emplace_back(ScriptCanvas::NodeUtils::ConstructSendScriptEventIdentifier(scriptEventTreeItem->GetBusIdentifier(), scriptEventTreeItem->GetEventIdentifier()));
        }
        else
        {
            nodeIdentifiers.emplace_back(ConstructNodeIdentifier(treeItem));
        }

        return nodeIdentifiers;
    }

    //////////////////////////
    // GraphStatisticsHelper
    //////////////////////////

    void GraphStatisticsHelper::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<GraphStatisticsHelper>()
                ->Version(1)
                ->Field("InstanceCounter", &GraphStatisticsHelper::m_nodeIdentifierCount)
            ;
        }
    }

    void GraphStatisticsHelper::PopulateStatisticData(const Graph* editorGraph)
    {
        // Opportunistically use this time to refresh out node count array.
        m_nodeIdentifierCount.clear();

        for (auto* nodeEntity : editorGraph->GetNodeEntities())
        {
            auto nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity);

            if (nodeComponent)
            {
                if (auto ebusHandlerNode  = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(nodeComponent))
                {
                    GraphCanvas::NodeId graphCanvasNodeId;
                    SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, nodeEntity->GetId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);                    

                    const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = ebusHandlerNode->GetEvents();

                    ScriptCanvas::EBusBusId busId = ebusHandlerNode->GetEBusId();

                    for (const auto& eventPair : events)
                    {
                        bool hasEvent = false;
                        EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventPair.second.m_eventId);

                        // In case we are populating from an uncreated scene if we don't have a valid graph canvas node id
                        // just accept everything. We can overreport on unknown data for now.
                        if (hasEvent || !graphCanvasNodeId.IsValid())
                        {
                            RegisterNodeType(ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(busId, eventPair.second.m_eventId));
                        }
                    }
                }
                else if (auto scriptEventHandler = azrtti_cast<ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(nodeComponent))
                {
                    GraphCanvas::NodeId graphCanvasNodeId;
                    SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, nodeEntity->GetId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                    EBusHandlerNodeDescriptorRequests* ebusDescriptorRequests = EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasNodeId);

                    if (ebusDescriptorRequests == nullptr)
                    {
                        continue;
                    }

                    AZStd::vector< HandlerEventConfiguration > eventConfigurations = ebusDescriptorRequests->GetEventConfigurations();

                    ScriptCanvas::EBusBusId busId = scriptEventHandler->GetBusId();

                    for (const auto& eventConfiguration : eventConfigurations)
                    {
                        bool hasEvent = ebusDescriptorRequests->ContainsEvent(eventConfiguration.m_eventId);

                        // In case we are populating from an uncreated scene if we don't have a valid graph canvas node id
                        // just accept everything. We can overreport on unknown data for now.
                        if (hasEvent)
                        {
                            RegisterNodeType(ScriptCanvas::NodeUtils::ConstructScriptEventReceiverIdentifier(busId, eventConfiguration.m_eventId));
                        }
                    }
                }
                else
                {
                    ScriptCanvas::NodeTypeIdentifier nodeType = nodeComponent->GetNodeType();

                    // Fallback in case something isn't initialized for whatever reason
                    if (nodeType == 0)
                    {
                        nodeType = ScriptCanvas::NodeUtils::ConstructNodeType(nodeComponent);
                    }

                    RegisterNodeType(nodeType);
                }
            }
        }
    }

    void GraphStatisticsHelper::RegisterNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        auto nodeIter = m_nodeIdentifierCount.find(nodeTypeIdentifier);

        if (nodeIter == m_nodeIdentifierCount.end())
        {
            m_nodeIdentifierCount.emplace(AZStd::make_pair(nodeTypeIdentifier, 1));
        }
        else
        {
            nodeIter->second++;
        }
    }
}
