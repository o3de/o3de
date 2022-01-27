/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QScopedValueRollback>
#include <QInputDialog>
#include <QFile>
#include <qmimedata.h>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/ConnectionBus.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/Nodes/NodeDisplayUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasAssetIdDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasBoolDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasEntityIdDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasEnumDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasNumericDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasColorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasCRCDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasReadOnlyDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasStringDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVectorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVariableDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasQuaternionDataInterface.h>

#include <Editor/GraphCanvas/PropertyInterfaces/ScriptCanvasEnumComboBoxPropertyDataInterface.h>
#include <Editor/GraphCanvas/PropertyInterfaces/ScriptCanvasStringPropertyDataInterface.h>

#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/MethodOverloaded.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/ReceiveScriptEvent.h>
#include <Libraries/Core/ScriptEventBase.h>
#include <ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>
#include <Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Libraries/UnitTesting/UnitTestingLibrary.h>

    AZ_CVAR(bool, g_disableDeprecatedNodeUpdates, false, {}, AZ::ConsoleFunctorFlags::Null,
        "Disables automatic update attempts of deprecated nodes, so that graphs that require and update can be viewed in their original form");

namespace EditorGraphCpp
{
    enum Version 
    {
        BeforeCovertedUnitTestNodes = 6,
        RemoveUnusedField,

        // label your entry above
        Current
    };
}
namespace ScriptCanvasEditor
{
    namespace EditorGraph
    {
        static const char* GetMimeType()
        {
            return "application/x-o3de-scriptcanvas";
        }

        static const char* GetWrappedNodeGroupingMimeType()
        {
            return "application/x-03de-scriptcanvas-wrappednodegrouping";
        }
    }

    Graph::~Graph()
    {
        for (auto& entry : m_graphCanvasSaveData)
        {
            delete entry.second;
        }

        m_graphCanvasSaveData.clear();

        delete m_graphCanvasSceneEntity;
        m_graphCanvasSceneEntity = nullptr;
    }

    static bool GraphVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootDataElementNode)
    {
        // Version 0/1 graph will have their SaveFormatConverted flag flipped off
        if (rootDataElementNode.GetVersion() < 2)
        {
            rootDataElementNode.AddElementWithData(context, "m_saveFormatConverted", false);
        }

        if (rootDataElementNode.GetVersion() < 6)
        {
            rootDataElementNode.AddElementWithData(context, "GraphCanvasSaveVersion", GraphCanvas::EntitySaveDataContainer::NoVersion);
        }

        if (rootDataElementNode.GetVersion() < 7)
        {
            rootDataElementNode.RemoveElementByName(AZ_CRC("m_pureDataNodesConvertedToVariables", 0x8823e2c4));
        }

        // Always check and remove this unused field to keep asset clean
        if (rootDataElementNode.FindElement(AZ_CRC("unitTestNodesConverted", 0x4389126a)) != -1)
        {
            rootDataElementNode.RemoveElementByName(AZ_CRC("unitTestNodesConverted", 0x4389126a));
        }
        return true;
    }

    void Graph::ConvertToGetVariableNode(Graph* graph, ScriptCanvas::VariableId variableId, const AZ::EntityId& nodeId, AZStd::unordered_map< AZ::EntityId, AZ::EntityId >& setVariableRemapping)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId = graph->GetScriptCanvasId();
        GraphCanvas::GraphId graphId = graph->GetGraphCanvasGraphId();

        AZ::EntityId gridId;
        GraphCanvas::SceneRequestBus::EventResult(gridId, graphId, &GraphCanvas::SceneRequests::GetGrid);

        AZ::Vector2 position;
        GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);

        int dataSlotIndex = 0;

        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > targetToNodeMapping;

        for (int i = 0; i < slotIds.size(); ++i)
        {
            AZ::EntityId slotId = slotIds[i];

            GraphCanvas::Endpoint endpoint(nodeId, slotId);

            AZStd::vector< AZ::EntityId > connectionIds;
            GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

            GraphCanvas::ConnectionType connectionType;
            GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

            GraphCanvas::SlotType slotType;
            GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                continue;
            }
            else if (slotType == GraphCanvas::SlotTypes::DataSlot)
            {
                ++dataSlotIndex;

                for (const AZ::EntityId& connectionId : connectionIds)
                {
                    GraphCanvas::Endpoint targetEndpoint;
                    GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                    AZ::EntityId targetNodeId = targetEndpoint.GetNodeId();

                    // Some nodes might have been converted
                    auto remappedNodeIter = setVariableRemapping.find(targetNodeId);
                    if (remappedNodeIter != setVariableRemapping.end())
                    {
                        targetNodeId = remappedNodeIter->second;

                        AZStd::vector< AZ::EntityId > originalSetDataSlots;
                        GraphCanvas::NodeRequestBus::EventResult(originalSetDataSlots, targetEndpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetSlotIds);

                        AZStd::vector< AZ::EntityId > newSetDataSlots;
                        GraphCanvas::NodeRequestBus::EventResult(newSetDataSlots, targetNodeId, &GraphCanvas::NodeRequests::GetSlotIds);

                        bool foundSlot = false;
                        int remappingDataSlotIndex = 0;

                        for (int ii = 0; ii < originalSetDataSlots.size(); ++ii)
                        {
                            GraphCanvas::SlotType originalSlotType = GraphCanvas::SlotTypes::Invalid;
                            GraphCanvas::SlotRequestBus::EventResult(originalSlotType, originalSetDataSlots[ii], &GraphCanvas::SlotRequests::GetSlotType);

                            if (originalSlotType == GraphCanvas::SlotTypes::DataSlot)
                            {
                                ++remappingDataSlotIndex;
                            }

                            if (originalSetDataSlots[ii] == targetEndpoint.m_slotId)
                            {
                                foundSlot = true;
                                break;
                            }
                        }

                        if (foundSlot)
                        {
                            for (int ii = 0; ii < newSetDataSlots.size(); ++ii)
                            {
                                GraphCanvas::SlotType remappedSlotType = GraphCanvas::SlotTypes::Invalid;
                                GraphCanvas::SlotRequestBus::EventResult(remappedSlotType, newSetDataSlots[ii], &GraphCanvas::SlotRequests::GetSlotType);

                                if (remappedSlotType == GraphCanvas::SlotTypes::DataSlot)
                                {
                                    --remappingDataSlotIndex;

                                    if (remappingDataSlotIndex == 0)
                                    {
                                        targetEndpoint = GraphCanvas::Endpoint(targetNodeId, newSetDataSlots[ii]);
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to convert a connection. Could not find equivalent connection pin on a converted Set Variable node.");
                            continue;
                        }
                    }

                    auto targetIter = targetToNodeMapping.find(targetNodeId);
                    AZStd::vector< AZ::EntityId > newSlotIds;
                    AZ::EntityId newNodeId;

                    if (targetIter == targetToNodeMapping.end())
                    {
                        NodeIdPair newVariablePair = Nodes::CreateGetVariableNode(variableId, scriptCanvasId);
                        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, newVariablePair.m_graphCanvasId, position, false);

                        AZ::Vector2 minorStep;
                        GraphCanvas::GridRequestBus::EventResult(minorStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                        position += minorStep;

                        GraphCanvas::NodeRequestBus::EventResult(newSlotIds, newVariablePair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetSlotIds);
                        newNodeId = newVariablePair.m_graphCanvasId;
                        targetToNodeMapping[targetNodeId] = newNodeId;

                        GraphCanvas::Endpoint newExecutionInEndpoint;
                        newExecutionInEndpoint.m_nodeId = newNodeId;

                        GraphCanvas::Endpoint newExecutionOutEndpoint;
                        newExecutionOutEndpoint.m_nodeId = newNodeId;

                        for (AZ::EntityId newSlotId : newSlotIds)
                        {
                            GraphCanvas::SlotType slotType2;
                            GraphCanvas::SlotRequestBus::EventResult(slotType2, newSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                            if (slotType2 == GraphCanvas::SlotTypes::ExecutionSlot)
                            {
                                GraphCanvas::ConnectionType connectionType2 = GraphCanvas::CT_Invalid;
                                GraphCanvas::SlotRequestBus::EventResult(connectionType2, newSlotId, &GraphCanvas::SlotRequests::GetConnectionType);

                                if (connectionType2 == GraphCanvas::CT_Input)
                                {
                                    newExecutionInEndpoint.m_slotId = newSlotId;
                                }
                                else if (connectionType2 == GraphCanvas::CT_Output)
                                {
                                    newExecutionOutEndpoint.m_slotId = newSlotId;
                                }
                            }
                        }

                        AZStd::vector< AZ::EntityId > targetSlotIds;
                        GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, targetNodeId, &GraphCanvas::NodeRequests::GetSlotIds);

                        bool spliceConnections = false;
                        AZ::EntityId targetExecutionInId;

                        for (AZ::EntityId testTargetSlotId : targetSlotIds)
                        {
                            GraphCanvas::SlotType slotType2;
                            GraphCanvas::SlotRequestBus::EventResult(slotType2, testTargetSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                            if (slotType2 != GraphCanvas::SlotTypes::ExecutionSlot)
                            {
                                continue;
                            }

                            GraphCanvas::ConnectionType connectionType2 = GraphCanvas::CT_Invalid;
                            GraphCanvas::SlotRequestBus::EventResult(connectionType2, testTargetSlotId, &GraphCanvas::SlotRequests::GetConnectionType);

                            if (connectionType2 == GraphCanvas::CT_Input)
                            {
                                bool hasConnections = false;
                                GraphCanvas::SlotRequestBus::EventResult(hasConnections, testTargetSlotId, &GraphCanvas::SlotRequests::HasConnections);

                                if (hasConnections)
                                {
                                    // Gate the connection, so we only try to splice connections if we have a single execution slot
                                    spliceConnections = !targetExecutionInId.IsValid();
                                    targetExecutionInId = testTargetSlotId;
                                }
                            }
                        }

                        if (spliceConnections)
                        {
                            AZStd::vector< AZ::EntityId > connectionIds2;
                            GraphCanvas::SlotRequestBus::EventResult(connectionIds2, targetExecutionInId, &GraphCanvas::SlotRequests::GetConnections);

                            GraphCanvas::Endpoint connectionTargetEndpoint(targetNodeId, targetExecutionInId);

                            bool createConnection = false;

                            for (const AZ::EntityId& oldConnectionId : connectionIds2)
                            {
                                GraphCanvas::Endpoint connectionSourceEndpoint;
                                GraphCanvas::ConnectionRequestBus::EventResult(connectionSourceEndpoint, oldConnectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                                if (graph->IsValidConnection(connectionSourceEndpoint, newExecutionInEndpoint))
                                {
                                    if (!createConnection)
                                    {
                                        createConnection = graph->IsValidConnection(newExecutionOutEndpoint, connectionTargetEndpoint);
                                    }

                                    AZStd::unordered_set<AZ::EntityId> deleteConnections = { oldConnectionId };
                                    GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteConnections);

                                    AZ::EntityId newConnectionId;
                                    GraphCanvas::SlotRequestBus::EventResult(newConnectionId, connectionSourceEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, newExecutionInEndpoint);

                                    if (newConnectionId.IsValid())
                                    {
                                        graph->CreateConnection(newConnectionId, connectionSourceEndpoint, newExecutionInEndpoint);
                                    }
                                }
                            }

                            if (createConnection)
                            {
                                AZ::EntityId newConnectionId;
                                GraphCanvas::SlotRequestBus::EventResult(newConnectionId, newExecutionOutEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, connectionTargetEndpoint);

                                if (newConnectionId.IsValid())
                                {
                                    graph->CreateConnection(newConnectionId, newExecutionOutEndpoint, connectionTargetEndpoint);
                                }
                            }
                        }
                    }
                    else
                    {
                        newNodeId = targetIter->second;
                        GraphCanvas::NodeRequestBus::EventResult(newSlotIds, newNodeId, &GraphCanvas::NodeRequests::GetSlotIds);
                    }

                    AZ::EntityId newSlotId;

                    // Going to just hope they're in the same ordering...since there really isn't much
                    // I can rely on to look this up.
                    int newDataSlotIndex = 0;

                    for (unsigned int newSlotIndex = 0; newSlotIndex < newSlotIds.size(); ++newSlotIndex)
                    {
                        AZ::EntityId testSlotId = newSlotIds[newSlotIndex];

                        GraphCanvas::SlotType slotType2;
                        GraphCanvas::SlotRequestBus::EventResult(slotType2, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                        if (slotType2 == GraphCanvas::SlotTypes::DataSlot)
                        {
                            ++newDataSlotIndex;

                            if (dataSlotIndex == newDataSlotIndex)
                            {
                                newSlotId = testSlotId;
                                break;
                            }
                        }
                    }

                    if (!newSlotId.IsValid() || !newNodeId.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", false, "Could not find appropriate Data Slot target when converting to a Get Variable node.");
                        continue;
                    }

                    // When stitching up the connections.
                    // We cannot add multiple data connections, so we need to remove the old connection before we attempt to make the
                    // new one, otherwise it might fail.
                    AZStd::unordered_set< AZ::EntityId > connectionClensing = { connectionId };
                    GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, connectionClensing);

                    GraphCanvas::Endpoint newEndpoint(newNodeId, newSlotId);

                    if (graph->IsValidConnection(newEndpoint, targetEndpoint))
                    {
                        AZ::EntityId newConnectionId;
                        GraphCanvas::SlotRequestBus::EventResult(newConnectionId, newEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

                        [[maybe_unused]] bool created = graph->CreateConnection(newConnectionId, newEndpoint, targetEndpoint);
                        AZ_Warning("ScriptCanvas", created, "Failed to created connection between migrated endpoints, despite valid connection check.");
                    }
                }
            }
        }
    }

    void Graph::Reflect(AZ::ReflectContext* context)
    {
        GraphStatisticsHelper::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CRCCache>()
                ->Version(1)
                ->Field("String", &CRCCache::m_cacheValue)
                ->Field("Count", &CRCCache::m_cacheCount)
                ;

            serializeContext->Class<Graph, ScriptCanvas::Graph>()
                ->Version(EditorGraphCpp::Version::Current, &GraphVersionConverter)
                ->Field("m_variableCounter", &Graph::m_variableCounter)
                ->Field("m_saveFormatConverted", &Graph::m_saveFormatConverted)
                ->Field("GraphCanvasData", &Graph::m_graphCanvasSaveData)
                ->Field("CRCCacheMap", &Graph::m_crcCacheMap)
                ->Field("StatisticsHelper", &Graph::m_statisticsHelper)
                ->Field("GraphCanvasSaveVersion", &Graph::m_graphCanvasSaveVersion)
                ;
        }
    }

    void Graph::Activate()
    {
        const ScriptCanvas::ScriptCanvasId& scriptCanvasId = GetScriptCanvasId();

        // Overridden to prevent graph execution in the editor
        NodeCreationNotificationBus::Handler::BusConnect(scriptCanvasId);
        SceneCounterRequestBus::Handler::BusConnect(scriptCanvasId);
        EditorGraphRequestBus::Handler::BusConnect(scriptCanvasId);
        ScriptCanvas::GraphRequestBus::Handler::BusConnect(scriptCanvasId);
        ScriptCanvas::StatusRequestBus::Handler::BusConnect(scriptCanvasId);
        GraphItemCommandNotificationBus::Handler::BusConnect(scriptCanvasId);
        GeneralEditorNotificationBus::Handler::BusConnect(scriptCanvasId);

        ScriptCanvas::Graph::Activate();
        PostActivate();
        m_undoHelper.SetSource(this);
    }

    void Graph::Deactivate()
    {
        GraphItemCommandNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::GraphRequestBus::Handler::BusDisconnect();
        EditorGraphRequestBus::Handler::BusDisconnect();
        SceneCounterRequestBus::Handler::BusDisconnect();
        NodeCreationNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect();

        delete m_graphCanvasSceneEntity;
        m_graphCanvasSceneEntity = nullptr;
    }
    
    void Graph::OnViewRegistered()
    {
        if (!m_saveFormatConverted)
        {
            ConstructSaveData();
        }
    }

    bool Graph::SanityCheckNodeReplacement(ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateSlotReport& nodeUpdateSlotReport)
    {
        auto findReplacementMatch = [](const ScriptCanvas::Slot* oldSlot, const AZStd::vector<const ScriptCanvas::Slot*>& newSlots)->ScriptCanvas::SlotId
        {
            for (auto& newSlot : newSlots)
            {
                if (newSlot->GetName() == oldSlot->GetName()
                && newSlot->GetType() == oldSlot->GetType()
                && (newSlot->IsExecution() || newSlot->GetDataType() == oldSlot->GetDataType()))
                {
                    return newSlot->GetId();
                }
            }

            return {};
        };

        if (!newNode)
        {
            AZ_Warning("ScriptCanvas", false, "Replacement node can not be null.");
            return false;
        }

        oldNode->CustomizeReplacementNode(newNode, nodeUpdateSlotReport.m_oldSlotsToNewSlots);

        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> slotNameMap = oldNode->GetReplacementSlotsMap();

        const auto newSlots = newNode->GetAllSlots();
        const auto oldSlots = oldNode->GetAllSlots();
        bool usingDefaults = true;
        size_t defaultMatchesFound = 0;

        auto& oldSlotsToNewSlots = nodeUpdateSlotReport.m_oldSlotsToNewSlots;

        for (auto oldSlot : oldSlots)
        {
            const ScriptCanvas::SlotId oldSlotId = oldSlot->GetId();
            const AZStd::string oldSlotName = oldSlot->GetName();

            auto slotIdsIter = oldSlotsToNewSlots.find(oldSlotId);
            auto slotNamesIter = slotNameMap.find(oldSlotName);
            // For old node slot remapping, we should get:
            // 1. if old slot name is not static, we should find the mapping in user provided slot id map
            // 2. if old slot name is static, we should find the mapping in codegen generated map (case 1 can override case 2)
            if (slotIdsIter != oldSlotsToNewSlots.end())
            {
                for (auto newSlotId : slotIdsIter->second)
                {
                    if (newSlotId.IsValid())
                    {
                        auto newSlot = newNode->GetSlot(newSlotId);
                        if (!newSlot)
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to find slot with id %s in replacement Node(%s).", newSlotId.ToString().c_str(), newNode->GetNodeName().c_str());
                            return false;
                        }
                        else if (newSlot && oldSlot->GetType() != newSlot->GetType())
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to map deprecated Node (%s) Slot (%s) to replacement Node (%s) Slot (%s).", oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str(), newNode->GetNodeName().c_str(), newSlot->GetName().c_str());
                            return false;
                        }
                    }
                }
            }
            else if (slotNamesIter != slotNameMap.end())
            {
                AZStd::vector<ScriptCanvas::SlotId> newSlotIds;
                for (auto newSlotName : slotNamesIter->second)
                {
                    if (!newSlotName.empty())
                    {
                        auto newSlot = newNode->GetSlotByName(newSlotName);

                        if (!newSlot)
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to find slot with name %s in replacement Node (%s).", newSlotName.c_str(), newNode->GetNodeName().c_str());
                            return false;
                        }
                        else if (newSlot && oldSlot->GetType() != newSlot->GetType())
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to map deprecated Node (%s) Slot (%s) to replacement Node (%s) Slot (%s).", oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str(), newNode->GetNodeName().c_str(), newSlot->GetName().c_str());
                            return false;
                        }

                        newSlotIds.push_back(newSlot->GetId());
                    }
                }
                oldSlotsToNewSlots.emplace(oldSlot->GetId(), newSlotIds);
            }
            else if (slotNameMap.empty())
            {
                usingDefaults = true;
                auto newSlotId = findReplacementMatch(oldSlot, newSlots);

                if (newSlotId.IsValid())
                {
                    ++defaultMatchesFound;
                    AZStd::vector<ScriptCanvas::SlotId> slotIds{ newSlotId };
                    oldSlotsToNewSlots.emplace(oldSlot->GetId(), slotIds);
                }
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Failed to remap deprecated Node(%s) Slot(%s).", oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str());
                return false;
            }
        }

        if (usingDefaults && oldSlotsToNewSlots.size() != oldSlots.size())
        {
            AZ_Warning("ScriptCanvas", false, "Failed to remap deprecated Node(%s) not all old slots were present in the new node.", oldNode->GetNodeName().c_str());
        }

        return true;
    }

    void Graph::HandleFunctionDefinitionExtension(ScriptCanvas::Node* node, GraphCanvas::SlotId graphCanvasSlotId, const GraphCanvas::NodeId& nodeId)
    {
        // Special-case for the execution nodeling extensions, which are adding input/output data slots.
        // We want to automatically promote them to variables so that the user can refer to them more easily
        auto functionDefintionNode = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node);
        if (functionDefintionNode && graphCanvasSlotId.IsValid())
        {
            GraphCanvas::Endpoint endpoint;
            GraphCanvas::SlotRequestBus::EventResult(endpoint, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetEndpoint);

            const ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
            if (scEndpoint.IsValid())
            {
                ScriptCanvas::Slot* slot = FindSlot(scEndpoint);

                if (slot)
                {
                    AZ::Vector2 position;
                    GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

                    // First we need to automatically display the ShowSlotTypeSelector dialog so the user
                    // can assign a type and name to the slot they are adding
                    VariablePaletteRequests::SlotSetup selectedSlotSetup;
                    bool createSlot = false;
                    QPoint scenePoint(aznumeric_cast<int>(position.GetX()), aznumeric_cast<int>(position.GetY()));
                    VariablePaletteRequestBus::BroadcastResult(createSlot, &VariablePaletteRequests::ShowSlotTypeSelector, slot, scenePoint, selectedSlotSetup);

                    if (createSlot && !selectedSlotSetup.m_type.IsNull())
                    {
                        if (slot)
                        {
                            auto displayType = ScriptCanvas::Data::FromAZType(selectedSlotSetup.m_type);
                            if (displayType.IsValid())
                            {
                                slot->SetDisplayType(displayType);
                            }

                            if (!selectedSlotSetup.m_name.empty())
                            {
                                slot->Rename(selectedSlotSetup.m_name);
                            }
                        }

                        // Now that the slot has a valid type/name, we can actually promote it to a variable
                        if (PromoteToVariableAction(endpoint) /*&& slot->IsVariableReference()*/)
                        {
                            ScriptCanvas::GraphVariable* variable = slot->GetVariable();

                            if (variable)
                            {
                                // functions 2.0 set variable scope to function 
                                if (variable->GetScope() != ScriptCanvas::VariableFlags::Scope::Function)
                                {
                                    variable->SetScope(ScriptCanvas::VariableFlags::Scope::Function);
                                }
                            }
                        }
                    }
                    else
                    {
                        RemoveSlot(endpoint);
                    }
                }
            }
        }
    }

    AZ::Outcome<ScriptCanvas::Node*> Graph::ReplaceNodeByConfig
        ( ScriptCanvas::Node* oldNode
        , const ScriptCanvas::NodeConfiguration& nodeConfig
        , ScriptCanvas::NodeUpdateSlotReport& nodeUpdateSlotReport)
    {
        auto nodeEntity = oldNode->GetEntity();
        if (!nodeEntity)
        {
            AZ_Warning("ScriptCanvas", false, "Could not find Node Entity for Node (%s).", oldNode->GetNodeName().c_str());
            return AZ::Failure();
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Warning("ScriptCanvas", false, "Failed to retrieve application serialize context.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeConfig.m_type);
        if (!classData)
        {
            AZ_Warning("ScriptCanvas", false, "Failed to find replacement class with UUID %s from serialize context.", nodeConfig.m_type.data);
            return AZ::Failure();
        }

        ScriptCanvas::Node* newNode = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name));
        if (!newNode)
        {
            AZ_Warning("ScriptCanvas", false, "Failed to create replacement Node (%s).", classData->m_name);
            return AZ::Failure();
        }

        bool rollbackRequired = false;
        nodeEntity->Deactivate();
        RemoveNode(oldNode->GetEntityId());
        nodeEntity->RemoveComponent(oldNode);

        nodeEntity->AddComponent(newNode);
        AddNode(newNode->GetEntityId());
        ScriptCanvas::NodeUtils::InitializeNode(newNode, nodeConfig);

        rollbackRequired = !SanityCheckNodeReplacement(oldNode, newNode, nodeUpdateSlotReport);
        auto& slotIdMap = nodeUpdateSlotReport.m_oldSlotsToNewSlots;

        if (rollbackRequired)
        {
            RemoveNode(newNode->GetEntityId());
            nodeEntity->RemoveComponent(newNode);
            delete newNode;

            nodeEntity->AddComponent(oldNode);
            AddNode(oldNode->GetEntityId());
            nodeEntity->Activate();
            return AZ::Failure();
        }
        else
        {
            nodeEntity->Activate();
            newNode->SignalReconfigurationBegin();
            newNode->SetNodeDisabledFlag(oldNode->GetNodeDisabledFlag());

            for (auto slotIdIter : slotIdMap)
            {
                ScriptCanvas::Slot* oldSlot = oldNode->GetSlot(slotIdIter.first);
                const ScriptCanvas::Endpoint oldEndpoint{ nodeEntity->GetId(), oldSlot->GetId() };
                if (slotIdIter.second.size() == 0)
                {
                    continue;
                }

                for (auto newSlotId : slotIdIter.second)
                {
                    ScriptCanvas::Slot* newSlot = nullptr;
                    if (newSlotId.IsValid())
                    {
                        newSlot = newNode->GetSlot(newSlotId);
                    }

                    // Copy over old slot data to new slot
                    if (newSlot && newSlot->GetDescriptor().IsData() && oldSlot->GetDescriptor().IsData())
                    {
                        AZ::Crc32 oldDynamicGroup = oldSlot->GetDynamicGroup();
                        ScriptCanvas::Data::Type oldDisplayType;
                        if (oldDynamicGroup != AZ::Crc32())
                        {
                            oldDisplayType = oldNode->GetDisplayType(oldDynamicGroup);
                        }
                        else
                        {
                            oldDisplayType = oldSlot->GetDataType();
                        }

                        if (oldDisplayType.IsValid())
                        {
                            AZ::Crc32 newDynamicGroup = newSlot->GetDynamicGroup();
                            if (newDynamicGroup != AZ::Crc32())
                            {
                                newNode->SetDisplayType(newDynamicGroup, oldDisplayType);
                            }
                            else
                            {
                                newSlot->ClearDisplayType();
                                newSlot->SetDisplayType(oldDisplayType);
                            }
                        }

                        ScriptCanvas::VersioningUtils::CopyOldValueToDataSlot(newSlot, oldSlot->GetVariableReference(), oldSlot->FindDatum());
                    }
                }
            }

            delete oldNode;
            newNode->SignalReconfigurationEnd();
            return AZ::Success(newNode);
        }
    }

    void Graph::OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget)
    {
        const GraphCanvas::GraphData& graphCanvasGraphData = serializationTarget.GetGraphData();

        AZStd::unordered_set<ScriptCanvas::VariableId> variableIds;
        AZStd::unordered_set< AZ::EntityId > forcedWrappedNodes;

        AZStd::unordered_set<AZ::Entity*> scriptCanvasEntities;

        for (const auto& node : graphCanvasGraphData.m_nodes)
        {
            // EBus Event nodes are purely visual, but require some user data manipulation in order to function correctly.
            // As such we don't want to copy over their script canvas user data, since it's not what was intended to be copied.
            if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(node->GetId()) == nullptr)
            {
                AZStd::any* userData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(userData, node->GetId(), &GraphCanvas::NodeRequests::GetUserData);
                auto scriptCanvasNodeId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
                AZ::Entity* scriptCanvasEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasNodeId);
                if (scriptCanvasEntity)
                {
                    scriptCanvasEntities.emplace(scriptCanvasEntity);

                    ScriptCanvas::Node* nodeComponent = FindNode(scriptCanvasEntity->GetId());

                    if (nodeComponent)
                    {
                        for (const auto& slot : nodeComponent->GetSlots())
                        {
                            if (slot.IsVariableReference())
                            {
                                variableIds.insert(slot.GetVariableReference());
                            }
                        }
                    }
                }

                if (GraphCanvas::ForcedWrappedNodeRequestBus::FindFirstHandler(node->GetId()) != nullptr)
                {
                    forcedWrappedNodes.insert(node->GetId());
                }
            }
            else
            {
                forcedWrappedNodes.insert(node->GetId());
            }
        }

        if (!variableIds.empty())
        {
            auto& userDataMapRef = serializationTarget.GetUserDataMapRef();

            auto mapIter = userDataMapRef.find(ScriptCanvas::CopiedVariableData::k_variableKey);

            ScriptCanvas::GraphVariableMapping* variableConfigurations = nullptr;

            if (mapIter == userDataMapRef.end())
            {
                ScriptCanvas::CopiedVariableData variableData;
                auto insertResult = userDataMapRef.emplace(ScriptCanvas::CopiedVariableData::k_variableKey, variableData);

                ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&insertResult.first->second);
                variableConfigurations = (&copiedVariableData->m_variableMapping);
            }
            else
            {
                ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&mapIter->second);
                variableConfigurations = (&copiedVariableData->m_variableMapping);
            }

            for (const auto& variableId : variableIds)
            {
                if (variableConfigurations->find(variableId) == variableConfigurations->end())
                {
                    ScriptCanvas::ScriptCanvasId scriptCanvasId;
                    GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetActiveScriptCanvasId);

                    ScriptCanvas::GraphVariable* configuration = nullptr;
                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(configuration, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

                    if (configuration)
                    {
                        variableConfigurations->emplace(variableId, (*configuration));
                    }
                }
            }
        }

        for (const auto& connection : graphCanvasGraphData.m_connections)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::ConnectionRequestBus::EventResult(userData, connection->GetId(), &GraphCanvas::ConnectionRequests::GetUserData);

            auto scriptCanvasConnectionId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            AZ::Entity* scriptCanvasEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasConnectionId);
            if (scriptCanvasEntity)
            {
                scriptCanvasEntities.emplace(scriptCanvasEntity);
            }
        }

        auto& userDataMap = serializationTarget.GetUserDataMapRef();

        AZStd::unordered_set<AZ::Entity*> graphData = CopyItems(scriptCanvasEntities);
        userDataMap.emplace(EditorGraph::GetMimeType(), graphData);

        if (!forcedWrappedNodes.empty())
        {
            // Keep track of which ebus methods were grouped together when we serialized them out.
            // This is so when we recreate them, we can create the appropriate number of
            // EBus wrappers and put the correct methods into each.
            WrappedNodeGroupingMap forcedWrappedNodeGroupings;

            for (const AZ::EntityId& wrappedNode : forcedWrappedNodes)
            {
                AZ::EntityId wrapperNode;
                GraphCanvas::NodeRequestBus::EventResult(wrapperNode, wrappedNode, &GraphCanvas::NodeRequests::GetWrappingNode);

                if (wrapperNode.IsValid())
                {
                    forcedWrappedNodeGroupings.emplace(wrappedNode, wrapperNode);
                }
            }

            userDataMap.emplace(EditorGraph::GetWrappedNodeGroupingMimeType(), forcedWrappedNodeGroupings);
        }
    }

    void Graph::OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource)
    {
        const auto& userDataMap = serializationSource.GetUserDataMapRef();

        auto userDataIt = userDataMap.find(EditorGraph::GetMimeType());
        if (userDataIt != userDataMap.end())
        {
            auto graphEntities(AZStd::any_cast<AZStd::unordered_set<AZ::Entity*>>(&userDataIt->second));
            if (graphEntities)
            {
                AddItems(*graphEntities);

                const ScriptCanvas::GraphVariableMapping* variableMapping = nullptr;

                userDataIt = userDataMap.find(ScriptCanvas::CopiedVariableData::k_variableKey);

                if (userDataIt != userDataMap.end())
                {
                    const ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&userDataIt->second);
                    variableMapping = (&copiedVariableData->m_variableMapping);
                }

                if (variableMapping)
                {
                    for (AZ::Entity* entity : (*graphEntities))
                    {
                        ScriptCanvas::Node* node = FindNode(entity->GetId());

                        if (node)
                        {
                            for (const auto& slot : node->GetSlots())
                            {
                                if (slot.IsVariableReference())
                                {
                                    ScriptCanvas::VariableId originalId = slot.GetVariableReference();

                                    ScriptCanvas::GraphVariable* variable = FindVariableById(originalId);

                                    if (variable == nullptr)
                                    {
                                        auto variableIter = variableMapping->find(originalId);

                                        if (variableIter != variableMapping->end())
                                        {
                                            const ScriptCanvas::GraphVariable& variableConfiguration = variableIter->second;

                                            AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> remapVariableOutcome = AZ::Failure(AZStd::string());
                                            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(remapVariableOutcome, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::RemapVariable, variableConfiguration);                                            

                                            if (remapVariableOutcome)
                                            {
                                                node->SetSlotVariableId(slot.GetId(), remapVariableOutcome.GetValue());
                                            }
                                            else
                                            {
                                                node->ClearSlotVariableId(slot.GetId());
                                            }
                                        }
                                        else
                                        {
                                            node->ClearSlotVariableId(slot.GetId());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        userDataIt = userDataMap.find(EditorGraph::GetWrappedNodeGroupingMimeType());

        if (userDataIt != userDataMap.end())
        {
            // Serialization system handled remapping this map data so we can just insert them into our map.
            const WrappedNodeGroupingMap* wrappedNodeGroupings = AZStd::any_cast<WrappedNodeGroupingMap>(&userDataIt->second);
            m_wrappedNodeGroupings.insert(wrappedNodeGroupings->begin(), wrappedNodeGroupings->end());
        }

        const GraphCanvas::GraphData& sceneData = serializationSource.GetGraphData();
        for (auto nodeEntity : sceneData.m_nodes)
        {
            NodeCreationNotificationBus::Event(GetScriptCanvasId(), &NodeCreationNotifications::OnGraphCanvasNodeCreated, nodeEntity->GetId());
        }
    }

    void Graph::DisconnectConnection(const GraphCanvas::ConnectionId& connectionId)
    {
        AZStd::any* connectionUserData = nullptr;
        GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);
        auto scConnectionId = connectionUserData && connectionUserData->is<AZ::EntityId>()
            ? *AZStd::any_cast<AZ::EntityId>(connectionUserData)
            : AZ::EntityId();

        if (ScriptCanvas::Connection* connection = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(scConnectionId))
        {
            ScriptCanvas::GraphNotificationBus::Event
                ( GetScriptCanvasId()
                , &ScriptCanvas::GraphNotifications::OnDisonnectionComplete
                , connectionId);
            DisconnectById(scConnectionId);
        }
    }

    ScriptCanvas::DataPtr Graph::Create()
    {
        if (AZ::Entity* entity = aznew AZ::Entity("Script Canvas Graph"))
        {
            auto graph = entity->CreateComponent<ScriptCanvasEditor::Graph>();
            entity->CreateComponent<EditorGraphVariableManagerComponent>(graph->GetScriptCanvasId());

            if (ScriptCanvas::DataPtr data = aznew ScriptCanvas::ScriptCanvasData())
            {
                data->m_scriptCanvasEntity.reset(entity);
                graph->MarkOwnership(*data);
                entity->Init();
                entity->Activate();
                return data;
            }
        }

        return nullptr;
    }

    void Graph::MarkOwnership(ScriptCanvas::ScriptCanvasData& owner)
    {
        m_owner = &owner;
    }

    ScriptCanvas::DataPtr Graph::GetOwnership() const
    {
        return const_cast<Graph*>(this)->m_owner;
    }

    bool Graph::CreateConnection(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint)
    {
        if (!sourcePoint.IsValid() || !targetPoint.IsValid())
        {
            return false;
        }

        DisconnectConnection(connectionId);
        bool scConnected = false;

        ScriptCanvas::Endpoint scSourceEndpoint = ConvertToScriptCanvasEndpoint(sourcePoint);
        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        scConnected = ConnectByEndpoint(scSourceEndpoint, scTargetEndpoint);

        if (scConnected)
        {
            scConnected = ConfigureConnectionUserData(scSourceEndpoint, scTargetEndpoint, connectionId);
        }

        if (scConnected)
        {
            ScriptCanvas::GraphNotificationBus::Event(GetScriptCanvasId(), &ScriptCanvas::GraphNotifications::OnConnectionComplete, connectionId);
        }


        return scConnected;
    }

    bool Graph::IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const
    {
        ScriptCanvas::Endpoint scSourceEndpoint = ConvertToScriptCanvasEndpoint(sourcePoint);
        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        return CanCreateConnectionBetween(scSourceEndpoint, scTargetEndpoint).IsSuccess();
    }

    AZStd::string Graph::GetDataTypeString(const AZ::Uuid&)
    {
        // This is used by the default tooltip setting in GraphCanvas, returning an empty string
        // in order for tooltips to be fully controlled by ScriptCanvas
        return {};
    }

    void Graph::OnRemoveUnusedNodes()
    {
    }

    void Graph::OnRemoveUnusedElements()
    {
        RemoveUnusedVariables();
    }

    bool Graph::AllowReset(const GraphCanvas::Endpoint& endpoint) const
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);

        ScriptCanvas::Node* node = FindNode(scEndpoint.GetNodeId());

        if (node)
        {
            const ScriptCanvas::Slot* slot = node->GetSlot(scEndpoint.GetSlotId());

            if (slot)
            {
                if (slot->IsVariableReference())
                {
                    return true;
                }
                else
                {
                    const ScriptCanvas::Datum* datum = node->FindDatum(scEndpoint.GetSlotId());

                    if (datum)
                    {
                        // BCO's create a reference when set to default. Going to bypass them for now.
                        return ScriptCanvas::Data::IsValueType(datum->GetType());
                    }
                }
            }
        }

        return false;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const GraphCanvas::NodeId& nodeId, const GraphCanvas::SlotId& slotId) const
    {
        (void)dataType;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        AZStd::any* slotUserData = nullptr;
        GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotId, &GraphCanvas::SlotRequests::GetUserData);
        auto scriptCanvasSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

        return CreateDisplayPropertyForSlot(scriptCanvasNodeId, scriptCanvasSlotId);
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const GraphCanvas::NodeId& nodeId, const GraphCanvas::NodeId& slotId) const
    {
        (void)slotId;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        ScriptCanvas::Node* node = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(scriptCanvasNodeId);

        if (node)
        {
            ScriptCanvas::NodePropertyInterface* propertyInterface = node->GetPropertyInterface(propertyId);

            if (propertyInterface)
            {
                GraphCanvas::DataInterface* dataInterface = nullptr;
                GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

                if (auto comboBoxPropertyInterface = azrtti_cast<ScriptCanvas::ComboBoxPropertyInterface*>(propertyInterface))
                {
                    GraphCanvas::ComboBoxDataInterface* comboBoxInterface = nullptr;

                    if (propertyInterface->GetDataType() == ScriptCanvas::Data::Type::BehaviorContextObject(ScriptCanvas::EnumComboBoxNodePropertyInterface::k_EnumUUID))
                    {
                        comboBoxInterface = aznew ScriptCanvasEnumComboBoxPropertyDataInterface(scriptCanvasNodeId, static_cast<ScriptCanvas::EnumComboBoxNodePropertyInterface*>(propertyInterface));
                    }

                    if (comboBoxInterface)
                    {
                        dataInterface = comboBoxInterface;
                        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateComboBoxNodePropertyDisplay, comboBoxInterface);
                    }
                }
                else
                {
                    switch (propertyInterface->GetDataType().GetType())
                    {
                    case ScriptCanvas::Data::eType::String:
                        dataInterface = aznew ScriptCanvasStringPropertyDataInterface(scriptCanvasNodeId, static_cast<ScriptCanvas::TypedNodePropertyInterface<ScriptCanvas::Data::StringType>*>(propertyInterface));
                        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, static_cast<GraphCanvas::StringDataInterface*>(dataInterface));
                        break;
                    default:
                        break;
                    }

                }

                if (dataDisplay != nullptr)
                {
                    return dataDisplay;
                }

                delete dataInterface;
            }
        }

        return nullptr;
    }

    AZ::EntityId Graph::ConvertToScriptCanvasNodeId(const GraphCanvas::NodeId& nodeId) const
    {
        AZStd::any* userData = nullptr;

        GraphCanvas::NodeRequestBus::EventResult(userData, nodeId, &GraphCanvas::NodeRequests::GetUserData);

        return (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        ScriptCanvas::Slot* slot = nullptr;
        ScriptCanvas::NodeRequestBus::EventResult(slot, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetSlot, scriptCanvasSlotId);

        if (slot == nullptr)
        {
            return nullptr;
        }

        if (slot->IsVariableReference())
        {
            ScriptCanvasVariableReferenceDataInterface* dataInterface = aznew ScriptCanvasVariableReferenceDataInterface(&m_variableDataModel, GetScriptCanvasId(), scriptCanvasNodeId, scriptCanvasSlotId);
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateComboBoxNodePropertyDisplay, dataInterface);

            if (dataDisplay)
            {
                return dataDisplay;
            }

            delete dataInterface;
            return nullptr;
        }

        // ScriptCanvas has access to better typing information regarding the slots than is exposed to GraphCanvas.
        // So let ScriptCanvas check the types based on it's own information rather than relying on the information passed back from GraphCanvas.
        ScriptCanvas::Data::Type slotType = slot->GetDataType();

        {
            GraphCanvas::DataInterface* dataInterface = nullptr;
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

            if (slotType.IS_A(ScriptCanvas::Data::Type::Boolean()))
            {
                dataInterface = aznew ScriptCanvasBoolDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateBooleanNodePropertyDisplay, static_cast<ScriptCanvasBoolDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::Number()))
            {
                dataInterface = aznew ScriptCanvasNumericDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay, static_cast<ScriptCanvasNumericDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::String()))
            {
                dataInterface = aznew ScriptCanvasStringDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, static_cast<ScriptCanvasStringDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::EntityID()))
            {
                dataInterface = aznew ScriptCanvasEntityIdDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateEntityIdNodePropertyDisplay, static_cast<ScriptCanvasEntityIdDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector3::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector3()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector3, 3>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector2::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector2()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector2, 2>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector4::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector4()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector4, 4>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Quaternion::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Quaternion()))
            {
                dataInterface = aznew ScriptCanvasQuaternionDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Color::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Color()))
            {
                dataInterface = aznew ScriptCanvasColorDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::CRC()))
            {
                dataInterface = aznew ScriptCanvasCRCDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, static_cast<GraphCanvas::StringDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::AssetId()))
            {
                dataInterface = aznew ScriptCanvasAssetIdDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateAssetIdNodePropertyDisplay, static_cast<GraphCanvas::AssetIdDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(ScriptCanvas::GraphScopedVariableId::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasGraphScopedVariableDataInterface(&m_variableDataModel, GetScriptCanvasId(), scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateComboBoxNodePropertyDisplay, static_cast<GraphCanvas::ComboBoxDataInterface*>(dataInterface));
            }

            if (dataDisplay != nullptr)
            {
                return dataDisplay;
            }

            delete dataInterface;
        }

        return nullptr;
    }

    void Graph::SignalDirty()
    {
        SourceHandle handle(m_owner, {}, {});
        GeneralRequestBus::Broadcast(&GeneralRequests::SignalSceneDirty, handle);
    }

    void Graph::HighlightNodesByType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        for (const auto& nodePair : GetNodeMapping())
        {
            if (nodePair.second->GetNodeType() == nodeTypeIdentifier)
            {
                HighlightScriptCanvasEntity(nodePair.first);
            }
        }
    }

    void Graph::HighlightEBusNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId)
    {
        ScriptCanvas::NodeTypeIdentifier ebusIdentifier = ScriptCanvas::NodeUtils::ConstructEBusIdentifier(busId);

        for (const auto& nodePair : GetNodeMapping())
        {
            ScriptCanvas::Node* canvasNode = nodePair.second;

            if (canvasNode->GetNodeType() == ebusIdentifier)
            {
                AZ::EntityId graphCanvasNodeId;
                SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                bool hasEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);
                if (hasEvent)
                {
                    HighlightScriptCanvasEntity(canvasNode->GetEntityId());
                }
            }
        }
    }

    void Graph::HighlightScriptEventNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId)
    {
        ScriptCanvas::NodeTypeIdentifier sendScriptEventIdentifier = ScriptCanvas::NodeUtils::ConstructSendScriptEventIdentifier(busId, eventId);
        ScriptCanvas::NodeTypeIdentifier receiveScriptEventIdentifier = ScriptCanvas::NodeUtils::ConstructScriptEventIdentifier(busId);

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

            if (canvasNode->GetNodeType() == sendScriptEventIdentifier)
            {
                HighlightScriptCanvasEntity(entity->GetId());
            }
            else if (canvasNode->GetNodeType() == receiveScriptEventIdentifier)
            {
                AZ::EntityId graphCanvasNodeId;
                SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                bool hasEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);
                if (hasEvent)
                {
                    HighlightScriptCanvasEntity(entity->GetId());
                }
            }
        }
    }

    void Graph::HighlightScriptCanvasEntity(const AZ::EntityId& scriptCanvasId)
    {
        GraphCanvas::SceneMemberGlowOutlineConfiguration glowConfiguration;

        glowConfiguration.m_blurRadius = 5;

        glowConfiguration.m_pen = QPen();
        glowConfiguration.m_pen.setBrush(QColor(243,129,29));
        glowConfiguration.m_pen.setWidth(5);

        SceneMemberMappingRequestBus::EventResult(glowConfiguration.m_sceneMember, scriptCanvasId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        glowConfiguration.m_pulseRate = AZStd::chrono::milliseconds(2500);
        glowConfiguration.m_zValue = 0;

        GraphCanvas::GraphicsEffectId graphicsEffectId;
        GraphCanvas::SceneRequestBus::EventResult(graphicsEffectId, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::CreateGlowOnSceneMember, glowConfiguration);

        if (graphicsEffectId.IsValid())
        {
            m_highlights.insert(graphicsEffectId);
        }
    }

    AZ::EntityId Graph::FindGraphCanvasSlotId(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId)
    {
        AZ::EntityId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, slotId);

        if (!graphCanvasSlotId.IsValid())
        {
            // For the EBusHandler's I need to remap these to a different visual node.
            // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
            if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasNodeId) != nullptr)
            {
                GraphCanvas::Endpoint graphCanvasEventEndpoint;
                EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasEventEndpoint, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, slotId);

                graphCanvasSlotId = graphCanvasEventEndpoint.GetSlotId();
            }
        }

        return graphCanvasSlotId;
    }

    bool Graph::ConfigureConnectionUserData(const ScriptCanvas::Endpoint& sourceEndpoint, const ScriptCanvas::Endpoint& targetEndpoint, GraphCanvas::ConnectionId connectionId)
    {
        bool isConfigured = true;

        AZ::Entity* scConnectionEntity = nullptr;
        FindConnection(scConnectionEntity, sourceEndpoint, targetEndpoint);

        if (scConnectionEntity)
        {
            AZStd::any* connectionUserData = nullptr;
            GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);

            if (connectionUserData)
            {
                *connectionUserData = scConnectionEntity->GetId();
                SceneMemberMappingConfigurationRequestBus::Event(connectionId, &SceneMemberMappingConfigurationRequests::ConfigureMapping, scConnectionEntity->GetId());
            }
        }
        else
        {
            isConfigured = false;
        }

        return isConfigured;
    }

    void Graph::HandleQueuedUpdates()
    {
        bool signalDirty = false;

        RequestPushPreventUndoStateUpdate();
        AZStd::unordered_set< ScriptCanvas::Node* > newUpdates;

        bool updatePropertyGrid = false;

        for (const AZ::EntityId& queuedUpdate : m_queuedConvertingNodes)
        {
            bool isSelected = false;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(isSelected, queuedUpdate, &GraphCanvas::SceneMemberUIRequests::IsSelected);

            if (isSelected)
            {
                updatePropertyGrid = true;
            }

            AZ::EntityId scriptCanvasNodeId = ConvertToScriptCanvasNodeId(queuedUpdate);
            ScriptCanvas::Node* node = FindNode(scriptCanvasNodeId);

            if (node->IsOutOfDate(GetVersion()))
            {
                if (OnVersionConversionBegin((*node)))
                {
                    newUpdates.insert(node);
                }
            }
        }

        m_queuedConvertingNodes.clear();

        AZStd::unordered_set<AZ::EntityId> deletedNodes;
        
        for (ScriptCanvas::Node* node : newUpdates)
        {
            ScriptCanvas::UpdateResult updateResult = node->UpdateNode();
            OnVersionConversionEnd((*node));

            AZ::EntityId graphCanvasNodeId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, node->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);
            switch (updateResult)
            {
                case ScriptCanvas::UpdateResult::DeleteNode:
                {
                    if (graphCanvasNodeId.IsValid())
                    {
                        deletedNodes.insert(graphCanvasNodeId);
                    }

                    signalDirty = true;
                    break;
                }
                default:
                {
                    signalDirty = true;
                    break;
                }
            }
        }

        if (!deletedNodes.empty())
        {
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, deletedNodes);
        }

        RequestPopPreventUndoStateUpdate();

        if (signalDirty)
        {
            SignalDirty();
        }

        if (updatePropertyGrid)
        {
            PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
        }
    }

    bool Graph::IsNodeVersionConverting(const AZ::EntityId& graphCanvasNodeId) const
    {
        bool isConverting = false;

        if (!m_convertingNodes.empty())
        {
            if (GraphCanvas::GraphUtils::IsNodeWrapped(graphCanvasNodeId))
            {
                AZ::EntityId parentId;
                GraphCanvas::NodeRequestBus::EventResult(parentId, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetWrappingNode);

                if (m_convertingNodes.count(parentId) > 0)
                {
                    isConverting = true;
                }
            }
            else if (m_convertingNodes.count(graphCanvasNodeId) > 0)
            {
                isConverting = true;
            }
        }

        return isConverting;
    }

    void Graph::OnPreNodeDeleted(const AZ::EntityId& nodeId)
    {
        // If we are cdeleteing a HandlerEventNode we don't need to do anything since they are purely visual.
        // And the underlying ScriptCanvas nodes will persist and maintain all of their state.
        if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(nodeId) == nullptr)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, nodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (userData && userData->is<AZ::EntityId>())
            {
                const AZ::EntityId* scriptCanvasNodeId = AZStd::any_cast<AZ::EntityId>(userData);

                auto iter = m_graphCanvasSaveData.find((*scriptCanvasNodeId));
                if (iter != m_graphCanvasSaveData.end())
                {
                    delete iter->second;
                    m_graphCanvasSaveData.erase(iter);
                }
            }

            AZStd::any* sourceUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(sourceUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
            auto scriptCanvasNodeId = sourceUserData && sourceUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(sourceUserData) : AZ::EntityId();

            if (RemoveNode(scriptCanvasNodeId))
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, scriptCanvasNodeId);
            }
        }
    }

    void Graph::OnPreConnectionDeleted(const AZ::EntityId& connectionId)
    {        
        AZStd::any* userData = nullptr;
        GraphCanvas::ConnectionRequestBus::EventResult(userData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            const AZ::EntityId* scriptCanvasConnectionId = AZStd::any_cast<AZ::EntityId>(userData);

            auto iter = m_graphCanvasSaveData.find((*scriptCanvasConnectionId));
            if (iter != m_graphCanvasSaveData.end())
            {
                delete iter->second;
                m_graphCanvasSaveData.erase(iter);
            }
        }

        GraphCanvas::Endpoint sourceEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

        ScriptCanvas::Endpoint scriptCanvasEndpoint = ConvertToScriptCanvasEndpoint(sourceEndpoint);

        // Don't disconnect any connections if we are version converting a node involved
        if (IsNodeVersionConverting(scriptCanvasEndpoint.GetNodeId()))
        {
            return;
        }

        GraphCanvas::Endpoint targetEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

        scriptCanvasEndpoint = ConvertToScriptCanvasEndpoint(targetEndpoint);

        if (IsNodeVersionConverting(scriptCanvasEndpoint.GetNodeId()))
        {
            return;
        }

        DisconnectConnection(connectionId);
    }

    void Graph::OnUnknownPaste([[maybe_unused]] const QPointF& scenePos)
    {
        GraphVariablesTableView::HandleVariablePaste(GetScriptCanvasId());
    }

    void Graph::OnSelectionChanged()
    {
        ClearHighlights();
    }

    AZ::u32 Graph::GetNewVariableCounter()
    {
        return ++m_variableCounter;
    }

    void Graph::ReleaseVariableCounter(AZ::u32 variableCounter)
    {
        if (m_variableCounter == variableCounter)
        {
            --m_variableCounter;
        }
    }

    void Graph::RequestUndoPoint()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
    }

    void Graph::RequestPushPreventUndoStateUpdate()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
    }

    void Graph::RequestPopPreventUndoStateUpdate()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
    }

    void Graph::TriggerUndo()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::TriggerUndo);
    }

    void Graph::TriggerRedo()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::TriggerRedo);
    }

    void Graph::EnableNodes(const AZStd::unordered_set< GraphCanvas::NodeId >& nodeIds)
    {
        bool enabledNodes = false;
        for (auto graphCanvasNodeId : nodeIds)
        {
            AZStd::any* nodeUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(nodeUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (auto* scNodeId = AZStd::any_cast<AZ::EntityId>(nodeUserData))
            {
                bool hasNonUserDisabledFlag = false;
                ScriptCanvas::NodeRequestBus::EventResult(hasNonUserDisabledFlag, (*scNodeId), &ScriptCanvas::NodeRequests::HasNodeDisabledFlag, ScriptCanvas::NodeDisabledFlag::NonUser);
                if (!hasNonUserDisabledFlag)
                {
                    ScriptCanvas::NodeRequestBus::Event((*scNodeId), &ScriptCanvas::NodeRequests::RemoveNodeDisabledFlag, ScriptCanvas::NodeDisabledFlag::User);
                    enabledNodes = true;
                }
            }            
        }

        if (enabledNodes)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
        }
    }

    void Graph::DisableNodes(const AZStd::unordered_set< GraphCanvas::NodeId >& nodeIds)
    {
        bool disabledNodes = false;
        for (auto graphCanvasNodeId : nodeIds)
        {
            AZStd::any* nodeUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(nodeUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (auto* scNodeId = AZStd::any_cast<AZ::EntityId>(nodeUserData))
            {
                ScriptCanvas::NodeRequestBus::Event((*scNodeId), &ScriptCanvas::NodeRequests::AddNodeDisabledFlag, ScriptCanvas::NodeDisabledFlag::User);
                disabledNodes = true;
            }
        }

        if (disabledNodes)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
        }
    }

    void Graph::PostDeletionEvent()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
    }

    void Graph::PostCreationEvent()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
        if (m_wrapperNodeDropTarget.IsValid())
        {
            for (const AZ::EntityId& nodeId : m_lastGraphCanvasCreationGroup)
            {
                GraphCanvas::WrappedNodeConfiguration configuration;
                GraphCanvas::WrapperNodeConfigurationRequestBus::EventResult(configuration, m_wrapperNodeDropTarget, &GraphCanvas::WrapperNodeConfigurationRequests::GetWrappedNodeConfiguration, nodeId);

                GraphCanvas::WrapperNodeRequestBus::Event(m_wrapperNodeDropTarget, &GraphCanvas::WrapperNodeRequests::WrapNode, nodeId, configuration);
            }
        }
        else
        {
            // List of nodes we want to delete, because they are invalid in our current context
            AZStd::unordered_set<AZ::EntityId> invalidNodes;

            // Three maps here.
            // WrapperTypeMapping: Keeps track of which wrappers were created by wrapper type.
            AZStd::unordered_map< AZ::Crc32, AZ::EntityId > wrapperTypeMapping;

            // WrapperIdMapping: Keeps track of EntityId mappings for the Wrappers.
            AZStd::unordered_map< AZ::EntityId, AZ::EntityId > wrapperIdMapping;

            // RequiredWrappers: Keeps track of a map of all of the wrapper types required to be created, along with the nodes
            //                   that wanted to create the nodes.
            AZStd::unordered_multimap< AZ::Crc32, AZ::EntityId > requiredWrappersMapping;

            // In general, we will only ever use 2 at once(in the case of a drag/drop: busType + eventWrapper)
            // In the case of a paste: busIdWrappers + eventWrappers
            // Logic is merged here just to try to reduce the duplicated logic, and because I can't really
            // tell the difference between the two cases anyway.
            //
            // Idea here is to keep track of groupings so that when we paste, I can create the appropriate number
            // of nodes and groupings within these nodes to create a proper duplicate. And when we drag and drop
            // I want to merge as many wrapped elements onto a single node as I can.
            //
            // First step in this process is to sort our pasted nodes into EBus handlers and EBus events.
            for (const AZ::EntityId& nodeId : m_lastGraphCanvasCreationGroup)
            {
                bool isExecutionNodeling = false;
                NodeDescriptorRequestBus::EventResult(isExecutionNodeling, nodeId, &NodeDescriptorRequests::IsType, NodeDescriptorType::FunctionDefinitionNode);

                if (isExecutionNodeling)
                {
                    AZStd::any* userData = nullptr;
                    GraphCanvas::NodeRequestBus::EventResult(userData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
                    AZ::EntityId scSourceNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
                        
                        ScriptCanvas::Nodes::Core::FunctionDefinitionNode* nodeling = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(FindNode(scSourceNodeId));

                    if (nodeling)
                    {
                        nodeling->RemapId();
                    }
                }

                bool isFunctionNode = false;
                NodeDescriptorRequestBus::EventResult(isFunctionNode, nodeId, &NodeDescriptorRequests::IsType, NodeDescriptorType::FunctionNode);

                // Show all hidden slots on a paste, as a temporary fix until I can sort out what I want this to work like.
                GraphCanvas::NodeRequestBus::Event(nodeId, &GraphCanvas::NodeRequests::ShowAllSlots);

                if (GraphCanvas::WrapperNodeRequestBus::FindFirstHandler(nodeId) != nullptr)
                {
                    wrapperIdMapping[nodeId] = nodeId;

                    AZ::Crc32 wrapperType;
                    GraphCanvas::WrapperNodeRequestBus::EventResult(wrapperType, nodeId, &GraphCanvas::WrapperNodeRequests::GetWrapperType);

                    if (wrapperType != AZ::Crc32())
                    {
                        auto mapIter = wrapperTypeMapping.find(wrapperType);

                        if (mapIter == wrapperTypeMapping.end())
                        {
                            wrapperTypeMapping[wrapperType] = nodeId;
                        }
                    }
                }

                if (GraphCanvas::ForcedWrappedNodeRequestBus::FindFirstHandler(nodeId) != nullptr)
                {
                    bool isWrapped = false;

                    GraphCanvas::NodeRequestBus::EventResult(isWrapped, nodeId, &GraphCanvas::NodeRequests::IsWrapped);

                    if (!isWrapped)
                    {
                        AZ::Crc32 wrapperType;
                        GraphCanvas::ForcedWrappedNodeRequestBus::EventResult(wrapperType, nodeId, &GraphCanvas::ForcedWrappedNodeRequests::GetWrapperType);

                        if (wrapperType != AZ::Crc32())
                        {
                            requiredWrappersMapping.emplace(wrapperType,nodeId);
                        }
                    }
                }
            }

            // Second step is to go through, and determine which usage case is valid so we know how to filter down our events.
            // If we can't find a wrapper, or we can't create a handler for the wrapper. We need to delete it.
            for (const auto& mapPair : requiredWrappersMapping)
            {
                AZ::EntityId wrapperNodeId;

                // Look up in our previous group mapping to see if it belonged to a node previously
                // (i.e. copy + pasted node).
                AZ::EntityId previousGroupWrapperNodeId;

                auto mapIter = m_wrappedNodeGroupings.find(mapPair.second);

                if (mapIter != m_wrappedNodeGroupings.end())
                {
                    previousGroupWrapperNodeId = mapIter->second;

                    auto busIter = wrapperIdMapping.find(previousGroupWrapperNodeId);

                    if (busIter != wrapperIdMapping.end())
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                // We may have already found our target node.
                // If we have, bypass the creation step.
                if (!wrapperNodeId.IsValid())
                {
                    // If we haven't check if we match a type, or if our previous group wrapper node is valid.
                    // If we had a previous group. I need to create a wrapper for that group.
                    // If we didn't have a previous group, I want to just use the Bus name to find an appropriate grouping.
                    auto busIter = wrapperTypeMapping.find(mapPair.first);
                    if (busIter == wrapperTypeMapping.end() || previousGroupWrapperNodeId.IsValid())
                    {
                        AZ::EntityId forcedWrappedNodeId = mapPair.second;

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, forcedWrappedNodeId, &GraphCanvas::GeometryRequests::GetPosition);

                        GraphCanvas::ForcedWrappedNodeRequestBus::EventResult(wrapperNodeId, forcedWrappedNodeId, &GraphCanvas::ForcedWrappedNodeRequests::CreateWrapperNode, GetGraphCanvasGraphId(), position);

                        if (wrapperNodeId.IsValid())
                        {
                            m_lastGraphCanvasCreationGroup.emplace_back(wrapperNodeId);

                            if (!previousGroupWrapperNodeId.IsValid())
                            {
                                wrapperTypeMapping.emplace(mapPair.first, wrapperNodeId);
                            }
                            else
                            {
                                wrapperIdMapping.emplace(previousGroupWrapperNodeId, wrapperNodeId);
                            }
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to instantiate an Wrapper node with type: (%d)", mapPair.first);
                            invalidNodes.insert(mapPair.second);
                            continue;
                        }
                    }
                    else
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                GraphCanvas::WrappedNodeConfiguration configuration;
                GraphCanvas::WrapperNodeConfigurationRequestBus::EventResult(configuration, wrapperNodeId, &GraphCanvas::WrapperNodeConfigurationRequests::GetWrappedNodeConfiguration, mapPair.second);

                GraphCanvas::WrapperNodeRequestBus::Event(wrapperNodeId, &GraphCanvas::WrapperNodeRequests::WrapNode, mapPair.second, configuration);
            }

            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, invalidNodes);
        }

        ScriptCanvas::Node::ExploredDynamicGroupCache exploredCache;

        for (AZ::EntityId graphCanvasNodeId : m_lastGraphCanvasCreationGroup)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scSourceNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            if (scSourceNodeId.IsValid())
            {
                ScriptCanvas::Node* node = FindNode(scSourceNodeId);
                
                if (node)
                {
                    node->SanityCheckDynamicDisplay(exploredCache);
                    node->PostActivate();
                }
            }

            OnSaveDataDirtied(graphCanvasNodeId);
            Nodes::UpdateSlotDatumLabels(graphCanvasNodeId);
        }

        m_wrappedNodeGroupings.clear();
        m_lastGraphCanvasCreationGroup.clear();
        m_wrapperNodeDropTarget.SetInvalid();

        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
    }

    void Graph::PostRestore(const UndoData&)
    {
        AZStd::vector<AZ::EntityId> graphCanvasNodeIds;
        GraphCanvas::SceneRequestBus::EventResult(graphCanvasNodeIds, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetNodes);

        for (AZ::EntityId graphCanvasNodeId : graphCanvasNodeIds)
        {
            Nodes::UpdateSlotDatumLabels(graphCanvasNodeId);
        }

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::RefreshView);
    }

    void Graph::OnPasteBegin()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
    }

    void Graph::OnPasteEnd()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetScriptCanvasId());
    }

    void Graph::OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId)
    {
        m_lastGraphCanvasCreationGroup.emplace_back(nodeId);
    }

    void Graph::ResetSlotToDefaultValue(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);

        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            canvasNode->ResetSlotToDefaultValue(scEndpoint.GetSlotId());
        }
    }

    void Graph::ResetReference(const GraphCanvas::Endpoint& endpoint)
    {
        // ResetSlotToDefault deals with resetting the reference internal to the function call on the node.
        ResetSlotToDefaultValue(endpoint);
    }

    void Graph::ResetProperty(const GraphCanvas::NodeId& nodeId, const AZ::Crc32& propertyId)
    {
        AZ::EntityId scriptCanvasNodeId = ConvertToScriptCanvasNodeId(nodeId);
        ScriptCanvas::Node* canvasNode = FindNode(scriptCanvasNodeId);

        if (canvasNode)
        {
            canvasNode->ResetProperty(propertyId);
        }
    }

    void Graph::RemoveSlot(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);

        ScriptCanvas::Slot* slot = FindSlot(scEndpoint);
        if (slot)
        {
            ScriptCanvas::GraphVariable* variable = slot->GetVariable();
            if (variable && variable->GetScope() == ScriptCanvas::VariableFlags::Scope::Function)
            {
                bool success = false;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(success, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, variable->GetVariableId());
                if (!success)
                {
                    AZ_Assert(success, "Failed to remove variable that corresponds to this slot");
                }
           }
        }

        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            canvasNode->DeleteSlot(scEndpoint.GetSlotId());
        }
    }

    bool Graph::IsSlotRemovable(const GraphCanvas::Endpoint& endpoint) const
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);

        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            return canvasNode->CanDeleteSlot(scEndpoint.GetSlotId());
        }

        return false;
    }

    bool Graph::ConvertSlotToReference(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            return canvasNode->ConvertSlotToReference(scEndpoint.GetSlotId());
        }

        return false;
    }

    bool Graph::CanConvertSlotToReference(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            ScriptCanvas::Slot* slot = canvasNode->GetSlot(scEndpoint.GetSlotId());
            if (slot)
            {
                return slot->CanConvertToReference();
            }
        }

        return false;
    }

    GraphCanvas::CanHandleMimeEventOutcome Graph::CanHandleReferenceMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            ScriptCanvas::Slot* slot = canvasNode->GetSlot(scEndpoint.GetSlotId());

            if (slot->CanConvertToReference() || slot->IsVariableReference())
            {
                ScriptCanvas::VariableId variableId = GraphCanvas::QtMimeUtils::ExtractTypeFromMimeData<ScriptCanvas::VariableId>(mimeData, GraphCanvas::k_ReferenceMimeType);

                ScriptCanvas::GraphVariable* variable = FindVariableById(variableId);

                if (variable)
                {
                    return canvasNode->SlotAcceptsType(scEndpoint.GetSlotId(), variable->GetDataType());
                }
                else
                {
                    return AZ::Failure(AZStd::string("Unable to find variable"));
                }
            }
            else
            {
                return AZ::Failure(AZStd::string("Unable to convert slot to Reference"));
            }
        }

        return AZ::Failure(AZStd::string("Unable to find Node"));
    }

    bool Graph::HandleReferenceMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData)
    {
        bool handledEvent = false;

        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            ScriptCanvas::Slot* slot = canvasNode->GetSlot(scEndpoint.GetSlotId());

            if (slot->IsVariableReference())
            {
                ScriptCanvas::VariableId variableId = GraphCanvas::QtMimeUtils::ExtractTypeFromMimeData<ScriptCanvas::VariableId>(mimeData, GraphCanvas::k_ReferenceMimeType);

                if (variableId.IsValid())
                {
                    canvasNode->SetSlotVariableId(scEndpoint.GetSlotId(), variableId);
                    handledEvent = true;
                }
            }
        }

        return handledEvent;
    }

    bool Graph::CanPromoteToVariable(const GraphCanvas::Endpoint& endpoint) const
    {
        ScriptCanvas::Endpoint scriptCanvasEndpoint = ConvertToScriptCanvasEndpoint(endpoint);        
        auto activeSlot = FindSlot(scriptCanvasEndpoint);

        if (activeSlot && !activeSlot->IsVariableReference() && activeSlot->CanConvertToReference())
        {
            if (!activeSlot->IsDynamicSlot() || activeSlot->HasDisplayType())
            {
                bool isValidVariableType = false;
                VariablePaletteRequestBus::BroadcastResult(isValidVariableType, &VariablePaletteRequests::IsValidVariableType, activeSlot->GetDataType());

                return isValidVariableType;
            }
        }

        return false;
    }

    bool Graph::PromoteToVariableAction(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scriptCanvasEndpoint = ConvertToScriptCanvasEndpoint(endpoint);

        auto activeNode = FindNode(scriptCanvasEndpoint.GetNodeId());
        auto activeSlot = FindSlot(scriptCanvasEndpoint);

        if (activeNode == nullptr || activeSlot == nullptr)
        {
            return false;
        }

        if (activeSlot->IsVariableReference())
        {
            return false;
        }

        if (activeSlot->IsDynamicSlot() && !activeSlot->HasDisplayType())
        {
            return false;
        }

        const ScriptCanvas::Datum* activeDatum = activeNode->FindDatum(scriptCanvasEndpoint.GetSlotId());

        AZStd::string variableName = "";

        int variableCounter = 0;
        AZStd::string defaultName; 
        
        AZ::Outcome<void, ScriptCanvas::GraphVariableValidationErrorCode> hasValidDefault = AZ::Failure(ScriptCanvas::GraphVariableValidationErrorCode::Unknown);

        do
        {
            variableCounter = GetNewVariableCounter();

            defaultName = VariableDockWidget::ConstructDefaultVariableName(variableCounter);

            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(hasValidDefault, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::IsNameValid, defaultName);
        } while (!hasValidDefault);

        bool nameAvailable = false;

        QWidget* mainWindow = nullptr;
        UIRequestBus::BroadcastResult(mainWindow, &UIRequests::GetMainWindow);

        AZStd::string inBoxText = "";

        // Special case to try re-using the slot name if this is on an execution nodeling, since the user had just
        // given it a name already with the ShowSlotTypeSelector dialog
        if (azrtti_istypeof<ScriptCanvas::Nodes::Core::FunctionDefinitionNode>(activeNode))
        {
            variableName = activeSlot->GetName();
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::IsNameAvailable, variableName);
        }

        int nameCount = 0;
        while (!nameAvailable)
        {
            if (nameCount == 0)
            {
                variableName.append(AZStd::string::format(" (%d)", ++nameCount));
            }
            else
            {
                AZ::StringFunc::Replace(variableName, AZStd::string::format("(%d)", nameCount-1).c_str(), AZStd::string::format("(%d)", nameCount).c_str());
                ++nameCount;
            }

            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::IsNameAvailable, variableName);
        }

        activeSlot->Rename(variableName);

        ScriptCanvas::Datum variableDatum;

        if (activeDatum)
        {
            variableDatum.ReconfigureDatumTo((*activeDatum));
        }
        else
        {
            variableDatum.SetType(activeSlot->GetDataType());

            // BCO Objects are defaulted to a reference. Going to bypass them to avoid messing with variable
            // defaults.
            if (ScriptCanvas::Data::IsValueType(activeSlot->GetDataType()))
            {
                variableDatum.SetToDefaultValueOfType();
            }
        }

        AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> addOutcome;

        // #functions2 slot<->variable re-use the activeDatum, send the pointer (actually, all of the source slot information, and make a special conversion)
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(addOutcome, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::AddVariable, variableName, variableDatum, true);

        if (addOutcome.IsSuccess())
        {
            GraphCanvas::DataSlotRequestBus::Event(endpoint.GetSlotId(), &GraphCanvas::DataSlotRequests::ConvertToReference);

            activeSlot->SetVariableReference(addOutcome.GetValue());

        }

        return addOutcome.IsSuccess();
    }

    bool Graph::SynchronizeReferences(const GraphCanvas::Endpoint& referenceSource, const GraphCanvas::Endpoint& referenceTarget)
    {
        ScriptCanvas::Endpoint scriptCanvasSourceEndpoint = ConvertToScriptCanvasEndpoint(referenceSource);
        ScriptCanvas::Endpoint scriptCanvasTargetEndpoint = ConvertToScriptCanvasEndpoint(referenceTarget);

        auto sourceSlot = FindSlot(scriptCanvasSourceEndpoint);
        auto targetSlot = FindSlot(scriptCanvasTargetEndpoint);

        if (sourceSlot == nullptr
            || targetSlot == nullptr)
        {
            return false;
        }

        if (!sourceSlot->IsVariableReference())
        {
            return false;
        }

        if (sourceSlot->IsTypeMatchFor((*targetSlot)))
        {
            if (!targetSlot->IsVariableReference())
            {
                GraphCanvas::DataSlotRequestBus::Event(referenceTarget.GetSlotId(), &GraphCanvas::DataSlotRequests::ConvertToReference);
            }

            if (targetSlot->IsVariableReference())
            {
                ScriptCanvas::VariableId variableId = sourceSlot->GetVariableReference();
                targetSlot->SetVariableReference(variableId);

                return true;
            }
        }

        return false;
    }

    bool Graph::ConvertSlotToValue(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            return canvasNode->ConvertSlotToValue(scEndpoint.GetSlotId());
        }

        return false;
    }

    bool Graph::CanConvertSlotToValue(const GraphCanvas::Endpoint& endpoint)
    {
        ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
        ScriptCanvas::Node* canvasNode = FindNode(scEndpoint.GetNodeId());

        if (canvasNode)
        {
            ScriptCanvas::Slot* slot = canvasNode->GetSlot(scEndpoint.GetSlotId());
            return slot && slot->CanConvertToValue();
        }

        return false;
    }

    GraphCanvas::CanHandleMimeEventOutcome Graph::CanHandleValueMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData)
    {
        AZ_UNUSED(endpoint);
        AZ_UNUSED(mimeData);

        AZ_Assert(false, "Unimplemented drag and drop flow");
        
        return AZ::Failure(AZStd::string("Unimplemented drag and drop flow"));
    }

    bool Graph::HandleValueMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData)
    {
        AZ_UNUSED(endpoint);
        AZ_UNUSED(mimeData);

        return false;
    }

    GraphCanvas::SlotId Graph::RequestExtension(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId, GraphModelRequests::ExtensionRequestReason reason)
    {
        GraphCanvas::SlotId graphCanvasSlotId;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);

        if (auto* scNodeId = AZStd::any_cast<AZ::EntityId>(nodeUserData))
        {
            AZ::Entity* graphNodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(graphNodeEntity, &AZ::ComponentApplicationRequests::FindEntity, *scNodeId);

            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(graphNodeEntity);
            if (canvasNode)
            {
                auto functionDefintionNode = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(canvasNode);
                if (functionDefintionNode && reason == GraphModelRequests::ExtensionRequestReason::ConnectionProposal)
                {

                }
                else
                {
                    ScriptCanvas::SlotId slotId = canvasNode->HandleExtension(extenderId);
                    if (slotId.IsValid())
                    {
                        SlotMappingRequestBus::EventResult(graphCanvasSlotId, nodeId, &SlotMappingRequests::MapToGraphCanvasId, slotId);
                        HandleFunctionDefinitionExtension(canvasNode, graphCanvasSlotId, nodeId);
                    }
                }
            }
        }

        return graphCanvasSlotId;
    }

    void Graph::ExtensionCancelled(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId)
    {
        AZ::EntityId scNodeId = ConvertToScriptCanvasNodeId(nodeId);

        if (scNodeId.IsValid())
        {
            ScriptCanvas::Node* canvasNode = FindNode(scNodeId);

            if (canvasNode)
            {
                canvasNode->ExtensionCancelled(extenderId);
            }
        }
    }

    void Graph::FinalizeExtension(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId)
    {
        AZ::EntityId scNodeId = ConvertToScriptCanvasNodeId(nodeId);

        if (scNodeId.IsValid())
        {
            ScriptCanvas::Node* canvasNode = FindNode(scNodeId);

            if (canvasNode)
            {
                canvasNode->FinalizeExtension(extenderId);
            }
        }
    }

    bool Graph::ShouldWrapperAcceptDrop(const AZ::EntityId& wrapperNode, const QMimeData* mimeData) const
    {
        if (!mimeData->hasFormat(Widget::NodePaletteDockWidget::GetMimeType()))
        {
            return false;
        }

        // Deep mime inspection
        QByteArray arrayData = mimeData->data(Widget::NodePaletteDockWidget::GetMimeType());

        GraphCanvas::GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return false;
        }

        AZStd::string busName;
        EBusHandlerNodeDescriptorRequestBus::EventResult(busName, wrapperNode, &EBusHandlerNodeDescriptorRequests::GetBusName);

        for (GraphCanvas::GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
        {
            CreateEBusHandlerEventMimeEvent* createEbusMethodEvent = azrtti_cast<CreateEBusHandlerEventMimeEvent*>(mimeEvent);

            if (createEbusMethodEvent)
            {
                if (createEbusMethodEvent->GetBusName().compare(busName) != 0)
                {
                    return false;
                }

                bool containsEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(containsEvent, wrapperNode, &EBusHandlerNodeDescriptorRequests::ContainsEvent, createEbusMethodEvent->GetEventId());

                if (containsEvent)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    void Graph::AddWrapperDropTarget(const AZ::EntityId& wrapperNode)
    {
        if (!m_wrapperNodeDropTarget.IsValid())
        {
            m_wrapperNodeDropTarget = wrapperNode;
        }
    }

    void Graph::RemoveWrapperDropTarget(const AZ::EntityId& wrapperNode)
    {
        if (m_wrapperNodeDropTarget == wrapperNode)
        {
            m_wrapperNodeDropTarget.SetInvalid();
        }
    }

    GraphCanvas::GraphId Graph::GetGraphCanvasGraphId() const
    {
        if (m_saveFormatConverted)
        {
            if (m_graphCanvasSceneEntity)
            {
                return m_graphCanvasSceneEntity->GetId();
            }

            return AZ::EntityId();
        }
        else
        {
            return GetEntityId();
        }
    }

    NodeIdPair Graph::CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position)
    {
        CreateCustomNodeMimeEvent mimeEvent(typeId);

        AZ::Vector2 dropPosition = position;
        
        if (mimeEvent.ExecuteEvent(position, dropPosition, GetGraphCanvasGraphId()))
        {
            return mimeEvent.GetCreatedPair();
        }

        return NodeIdPair();
    }

    void Graph::AddCrcCache(const AZ::Crc32& crcValue, const AZStd::string& cacheString)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);

        if (mapIter == m_crcCacheMap.end())
        {
            m_crcCacheMap.emplace(crcValue, CRCCache(cacheString));
        }
        else
        {
            mapIter->second.m_cacheCount++;
        }
    }

    void Graph::RemoveCrcCache(const AZ::Crc32& crcValue)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);

        if (mapIter != m_crcCacheMap.end())
        {
            mapIter->second.m_cacheCount--;

            if (mapIter->second.m_cacheCount <= 0)
            {
                m_crcCacheMap.erase(mapIter);
            }
        }
    }

    AZStd::string Graph::DecodeCrc(const AZ::Crc32& crcValue)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);
        
        if (mapIter != m_crcCacheMap.end())
        {
            return mapIter->second.m_cacheValue;
        }

        return "";
    }

    void Graph::ClearHighlights()
    {
        for (const GraphCanvas::GraphicsEffectId& effectId : m_highlights)
        {
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::CancelGraphicsEffect, effectId);
        }
        
        m_highlights.clear();
    }

    void Graph::HighlightMembersFromTreeItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        ClearHighlights();

        if (auto handleEbusEventTreeItem = azrtti_cast<const EBusHandleEventPaletteTreeItem*>(treeItem))
        {
            HighlightEBusNodes(handleEbusEventTreeItem->GetBusId(), handleEbusEventTreeItem->GetEventId());
        }
        else if (auto sendScriptEventTreeItem = azrtti_cast<const ScriptEventsEventNodePaletteTreeItem*>(treeItem))
        {
            HighlightScriptEventNodes(sendScriptEventTreeItem->GetBusIdentifier(), sendScriptEventTreeItem->GetEventIdentifier());
        }
        else
        {
            HighlightNodesByType(NodeIdentifierFactory::ConstructNodeIdentifier(treeItem));
        }
    }

    void Graph::HighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
    {
        ClearHighlights();

        for (auto nodeComponentPair : GetNodeMapping())
        {
            ScriptCanvas::Node* node = nodeComponentPair.second;

            if (node->ContainsReferencesToVariables(variableIds))
            {
                HighlightScriptCanvasEntity(nodeComponentPair.first);
            }
        }
    }

    void Graph::HighlightNodes(const AZStd::vector<NodeIdPair>& nodes)
    {
        ClearHighlights();

        for (const NodeIdPair& nodeIdPair : nodes)
        {
            HighlightScriptCanvasEntity(nodeIdPair.m_scriptCanvasId);
        }
    }

    void Graph::RemoveUnusedVariables()
    {
        RequestPushPreventUndoStateUpdate();
        auto variableData = GetVariableData();

        auto variables = variableData->GetVariables();
        
        AZStd::unordered_set<ScriptCanvas::VariableId> usedVariableIds;

        for (auto nodePair : GetNodeMapping())
        {
            ScriptCanvas::Node* node = nodePair.second;
            node->CollectVariableReferences(usedVariableIds);
        }

        AZStd::unordered_set<ScriptCanvas::VariableId> unusedVariables;

        for (auto variableData2 : variables)
        {
            if (usedVariableIds.count(variableData2.first) == 0)
            {
                unusedVariables.insert(variableData2.first);
            }
        }

        bool removedVariable = false;

        for (ScriptCanvas::VariableId variableId : unusedVariables)
        {
            bool success = false;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(success, GetScriptCanvasId(), &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, variableId);

            if (success)
            {
                removedVariable = true;
            }
        }

        RequestPopPreventUndoStateUpdate();

        if (removedVariable)
        {
            RequestUndoPoint();
        }
    }

    bool Graph::CanConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId)
    {
        AZ::EntityId scriptCanvasNodeId = ConvertToScriptCanvasNodeId(nodeId);

        ScriptCanvas::VariableId variableId;
        ScriptCanvas::VariableNodeRequestBus::EventResult(variableId, scriptCanvasNodeId, &ScriptCanvas::VariableNodeRequests::GetId);

        ScriptCanvas::GraphVariable* variable = FindVariableById(variableId);

        if (variable == nullptr)
        {
            return false;
        }

        AZStd::vector< GraphCanvas::SlotId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);

        for (const GraphCanvas::SlotId& slotId : slotIds)
        {
            GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
            GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                continue;
            }

            GraphCanvas::Endpoint gcEndpoint(nodeId, slotId);
            ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(gcEndpoint);

            ScriptCanvas::Node* node = FindNode(scEndpoint.GetNodeId());

            // We only care about the actual variable type for enabling/disabling the button.
            // All other conditions will be handled in the conversion with user prompts.
            if (node->SlotAcceptsType(scEndpoint.GetSlotId(), variable->GetDataType()))
            {
                AZStd::vector< GraphCanvas::ConnectionId > connectionIds;
                GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

                return !connectionIds.empty();
            }
        }

        return false;
    }

    bool Graph::ConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId)
    {
        AZ::EntityId scriptCanvasNodeId = ConvertToScriptCanvasNodeId(nodeId);

        ScriptCanvas::VariableId variableId;
        ScriptCanvas::VariableNodeRequestBus::EventResult(variableId, scriptCanvasNodeId, &ScriptCanvas::VariableNodeRequests::GetId);

        ScriptCanvas::GraphVariable* variable = FindVariableById(variableId);

        if (variable == nullptr)
        {
            return false;
        }

        AZStd::vector< GraphCanvas::SlotId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);

        AZStd::unordered_set< GraphCanvas::Endpoint > referencableEndpoints;

        bool canDetachNode = true;

        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        QMainWindow* mainWindow = nullptr;
        UIRequestBus::BroadcastResult(mainWindow, &UIRequests::GetMainWindow);

        for (const GraphCanvas::SlotId& slotId : slotIds)
        {
            GraphCanvas::SlotRequests* slotRequests = GraphCanvas::SlotRequestBus::FindFirstHandler(slotId);

            if (slotRequests)
            {
                GraphCanvas::SlotType slotType = slotRequests->GetSlotType();

                if (slotType == GraphCanvas::SlotTypes::DataSlot)
                {
                    GraphCanvas::Endpoint currentEndpoint(nodeId, slotId);

                    // If we have a reference anywhere on us. We need to maintain this node, since it's not doing something
                    // we can merge out cleanly
                    ScriptCanvas::Endpoint scriptCanvasCurrentEndpoint = ConvertToScriptCanvasEndpoint(currentEndpoint);
                    ScriptCanvas::Slot* sourceSlot = FindSlot(scriptCanvasCurrentEndpoint);

                    if (sourceSlot->IsVariableReference())
                    {
                        canDetachNode = false;
                    }
                    
                    auto connectionIds = slotRequests->GetConnections();

                    for (auto connectionId : connectionIds)
                    {
                        GraphCanvas::Endpoint otherEndpoint;
                        GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, connectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, currentEndpoint);

                        ScriptCanvas::Endpoint scriptCanvasOtherEndpoint = ConvertToScriptCanvasEndpoint(otherEndpoint);
                        ScriptCanvas::Node* otherNode = FindNode(scriptCanvasOtherEndpoint.GetNodeId());
                        ScriptCanvas::Slot* otherSlot = FindSlot(scriptCanvasOtherEndpoint);

                        if (otherNode && otherSlot && otherNode->SlotAcceptsType(scriptCanvasOtherEndpoint.GetSlotId(), variable->GetDataType()))
                        {
                            AZStd::unordered_set< AZ::EntityId > deletedConnection = { connectionId };
                            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deletedConnection);

                            if (otherSlot->CanConvertToReference())
                            {
                                referencableEndpoints.insert(otherEndpoint);
                            }
                            else
                            {
                                // Try to resolve chained steps when we are going to end up being a 'Get' reference which is fine to convert to.
                                // Otherwise, if we chain to a 'set' reference, that might have unintended consequences, so we need to ignore that.
                                // Because we will double invert, check our source if we are an input. We can chain. If we are an output, we don't want to chain.
                                if (sourceSlot->IsInput())
                                {
                                    AZStd::vector< GraphCanvas::ConnectionId > chainedConnectionIds;
                                    GraphCanvas::SlotRequestBus::EventResult(chainedConnectionIds, otherEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetConnections);

                                    for (auto chainedConnectionId : chainedConnectionIds)
                                    {
                                        GraphCanvas::Endpoint chainedEndpoint;
                                        GraphCanvas::ConnectionRequestBus::EventResult(chainedEndpoint, chainedConnectionId, &GraphCanvas::ConnectionRequests::FindOtherEndpoint, otherEndpoint);

                                        ScriptCanvas::Endpoint scriptCanvasChainedEndpoint = ConvertToScriptCanvasEndpoint(chainedEndpoint);

                                        ScriptCanvas::Node* chainedNode = FindNode(scriptCanvasChainedEndpoint.GetNodeId());
                                        ScriptCanvas::Slot* chainedSlot = FindSlot(scriptCanvasChainedEndpoint);

                                        if (chainedNode && chainedSlot && chainedNode->SlotAcceptsType(scriptCanvasChainedEndpoint.GetSlotId(), variable->GetDataType()))
                                        {
                                            AZStd::unordered_set< AZ::EntityId > chainedDeletedConnection = { chainedConnectionId };
                                            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, chainedDeletedConnection);

                                            if (chainedSlot->CanConvertToReference())
                                            {
                                                referencableEndpoints.insert(chainedEndpoint);
                                            }
                                            else
                                            {
                                                GraphCanvas::SlotRequestBus::Event(chainedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, otherEndpoint);
                                            }
                                        }
                                    }
                                }

                                if (otherSlot->CanConvertToReference())
                                {
                                    referencableEndpoints.insert(otherEndpoint);
                                }
                                else
                                {
                                    GraphCanvas::SlotRequestBus::Event(currentEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, otherEndpoint);
                                    canDetachNode = false;
                                }
                            }
                        }
                        else
                        {
                            canDetachNode = false;
                        }
                    }
                }
            }
        }
        
        // Signal out on the graph that we did something to the node.
        GraphCanvas::AnimatedPulseConfiguration animatedPulseConfig;

        animatedPulseConfig.m_enableGradient = true;

        if (canDetachNode)
        {
            animatedPulseConfig.m_drawColor = QColor(255, 0, 0);
        }
        else
        {
            animatedPulseConfig.m_drawColor = QColor(255, 255, 255);
        }

        animatedPulseConfig.m_durationSec = 0.25f;

        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::CreatePulseAroundSceneMember, nodeId, 4, animatedPulseConfig);

        // If we can detach the node. All connections will be deleted, except for the ones we want to save.
        if (canDetachNode)
        {
            GraphCanvas::NodeDetachConfig detachConfig(nodeId);

            detachConfig.m_listingType = GraphCanvas::ListingType::InclusiveList;
            detachConfig.m_typeListing.insert(GraphCanvas::SlotTypes::ExecutionSlot);

            GraphCanvas::GraphUtils::DetachNodeAndStitchConnections(detachConfig);

            AZStd::unordered_set<GraphCanvas::NodeId > nodeIds = { nodeId };
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, nodeIds);
        }

        for (auto graphCanvasEndpoint : referencableEndpoints)
        {
            GraphCanvas::DataSlotRequestBus::Event(graphCanvasEndpoint.GetSlotId(), &GraphCanvas::DataSlotRequests::ConvertToReference);

            ScriptCanvas::Endpoint scriptCanvasEndpoint = ConvertToScriptCanvasEndpoint(graphCanvasEndpoint);

            ScriptCanvas::Slot* slot = FindSlot(scriptCanvasEndpoint);

            if (slot && slot->IsVariableReference())
            {
                slot->SetVariableReference(variable->GetVariableId());
            }
        }

        return true;
    }

    bool Graph::ConvertReferenceToVariableNode([[maybe_unused]] const GraphCanvas::Endpoint& endpoint)
    {
        return false;
    }

    bool Graph::OnVersionConversionBegin(ScriptCanvas::Node& scriptCanvasNode)
    {
        auto insertResult = m_convertingNodes.insert(scriptCanvasNode.GetEntityId());

        if (!insertResult.second)
        {
            return false;
        }

        for (const ScriptCanvas::Slot& currentSlot : scriptCanvasNode.GetSlots())
        {
            m_versionedSlots.insert(AZStd::make_pair(scriptCanvasNode.GetEntityId(), currentSlot.GetId()));
        }

        EditorNodeNotificationBus::Event(scriptCanvasNode.GetEntityId(), &EditorNodeNotifications::OnVersionConversionBegin);

        return true;
    }

    void Graph::OnVersionConversionEnd(ScriptCanvas::Node& scriptCanvasNode)
    {
        EditorNodeNotificationBus::Event(scriptCanvasNode.GetEntityId(), &EditorNodeNotifications::OnVersionConversionEnd);

        size_t removeCount = m_convertingNodes.count(scriptCanvasNode.GetEntityId());

        if (removeCount > 0)
        {
            auto findResult = m_versionedSlots.equal_range(scriptCanvasNode.GetEntityId());

            AZStd::unordered_set<ScriptCanvas::SlotId> previousSlots;

            for (auto cacheIter = findResult.first; cacheIter != findResult.second; ++cacheIter)
            {
                previousSlots.insert(cacheIter->second);
            }

            AZStd::unordered_set< GraphCanvas::ConnectionId > deletedGraphCanvasConnections;

            for (const ScriptCanvas::Slot& constantSlot : scriptCanvasNode.GetSlots())
            {
                ScriptCanvas::Slot* currentSlot = scriptCanvasNode.GetSlot(constantSlot.GetId());

                if (!currentSlot)
                {
                    AZ_Error("ScriptCanvas", false, "Missing slot from node %s after conversion ", scriptCanvasNode.GetDebugName().data());
                    continue;
                }

                ScriptCanvas::SlotId slotId = currentSlot->GetId();

                size_t eraseCount = previousSlots.erase(slotId);

                if (eraseCount == 0)
                {
                    continue;
                }

                // Manage updating connections and remove invalid ones
                ScriptCanvas::Endpoint endpoint = currentSlot->GetEndpoint();
                GraphCanvas::Endpoint graphCanvasEndpoint = ConvertToGraphCanvasEndpoint(endpoint);

                AZStd::vector< ScriptCanvas::Endpoint > connectedEndpoints = GetConnectedEndpoints(endpoint);

                for (const ScriptCanvas::Endpoint& connectedEndpoint : connectedEndpoints)
                {
                    if (IsNodeVersionConverting(connectedEndpoint.GetNodeId()))
                    {
                        continue;
                    }

                    bool allowConnection = CanConnectionExistBetween(connectedEndpoint, endpoint).IsSuccess();
                    bool deleteConnection = true;

                    if (graphCanvasEndpoint.IsValid())
                    {
                        GraphCanvas::Endpoint otherEndpoint = ConvertToGraphCanvasEndpoint(connectedEndpoint);

                        if (otherEndpoint.IsValid())
                        {
                            bool isConnected = false;
                            GraphCanvas::SlotRequestBus::EventResult(isConnected, graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::IsConnectedTo, otherEndpoint);

                            if (isConnected)
                            {
                                deleteConnection = false;

                                // If Graph canvas is connected, but we need to kill the connection
                                // we'll let the Graph Canvas Deletion Update our internal state.
                                if (!allowConnection)
                                {
                                    AZStd::unordered_set< GraphCanvas::Endpoint > searchEndpoints = { otherEndpoint };
                                    GraphCanvas::SlotRequestBus::Event(graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::FindConnectionsForEndpoints, searchEndpoints, deletedGraphCanvasConnections);
                                }
                            }
                            else if (allowConnection)
                            {
                                deleteConnection = false;
                                GraphCanvas::SlotRequestBus::Event(graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, otherEndpoint);
                            }
                        }
                    }

                    if (deleteConnection)
                    {
                        AZ::Entity* connectionEntity = nullptr;

                        if (FindConnection(connectionEntity, endpoint, connectedEndpoint))
                        {
                            RemoveConnection(connectionEntity->GetId());
                        }
                    }
                }
            }

            for (auto erasedSlot : previousSlots)
            {
                VersioningRemoveSlot(scriptCanvasNode, erasedSlot);
            }

            m_versionedSlots.erase(scriptCanvasNode.GetEntityId());
            m_convertingNodes.erase(scriptCanvasNode.GetEntityId());

            if (!deletedGraphCanvasConnections.empty())
            {
                GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, deletedGraphCanvasConnections);
            }

            AZStd::string updateString = scriptCanvasNode.GetUpdateString();
            m_updateStrings.insert(updateString);

            if (m_convertingNodes.empty())
            {
                DisplayUpdateToast();
            }
        }
    }

    AZStd::vector<NodeIdPair> Graph::GetNodesOfType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        AZStd::vector<NodeIdPair> nodeIdPairs;

        for (auto nodeMappingPair : GetNodeMapping())
        {
            ScriptCanvas::Node* canvasNode = nodeMappingPair.second;
            AZ::EntityId nodeEntityId = canvasNode->GetEntityId();

            if (canvasNode->GetNodeType() == nodeTypeIdentifier)
            {
                NodeIdPair nodeIdPair;
                nodeIdPair.m_scriptCanvasId = nodeEntityId;

                SceneMemberMappingRequestBus::EventResult(nodeIdPair.m_graphCanvasId, nodeEntityId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                nodeIdPairs.emplace_back(nodeIdPair);
            }
            else if (ScriptCanvas::Nodes::Core::EBusEventHandler* handlerNode = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(canvasNode))
            {
                ScriptCanvas::EBusBusId busId = handlerNode->GetEBusId();

                for (auto eventPair : handlerNode->GetEvents())
                {
                    ScriptCanvas::EBusEventId eventId = eventPair.second.m_eventId;

                    if (ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(busId, eventId) == nodeTypeIdentifier)
                    {
                        AZ::EntityId graphCanvasNodeId;
                        SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                        bool hasEvent = false;
                        EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);

                        if (hasEvent)
                        {
                            NodeIdPair nodeIdPair;
                            nodeIdPair.m_scriptCanvasId = nodeEntityId;
                            nodeIdPair.m_graphCanvasId = graphCanvasNodeId;

                            nodeIdPairs.emplace_back(nodeIdPair);
                        }
                    }
                }
            }
            else if (ScriptCanvas::Nodes::Core::ReceiveScriptEvent* receiveScriptEvent = azrtti_cast<ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(canvasNode))
            {
                AZ::EntityId graphCanvasNodeId;
                SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                EBusHandlerNodeDescriptorRequests* ebusHandlerDescriptor = EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasNodeId);

                if (ebusHandlerDescriptor)
                {
                    auto eventConfigurations = ebusHandlerDescriptor->GetEventConfigurations();

                    ScriptCanvas::EBusBusId busId = receiveScriptEvent->GetBusId();

                    for (auto eventConfiguration : eventConfigurations)
                    {
                        if (ScriptCanvas::NodeUtils::ConstructScriptEventReceiverIdentifier(busId, eventConfiguration.m_eventId) == nodeTypeIdentifier)
                        {
                            if (ebusHandlerDescriptor->ContainsEvent(eventConfiguration.m_eventId))
                            {
                                NodeIdPair nodeIdPair;
                                nodeIdPair.m_scriptCanvasId = nodeEntityId;
                                nodeIdPair.m_graphCanvasId = graphCanvasNodeId;

                                nodeIdPairs.emplace_back(nodeIdPair);
                            }
                        }
                    }
                }
            }
        }

        return nodeIdPairs;
    }

    AZStd::vector<NodeIdPair> Graph::GetVariableNodes(const ScriptCanvas::VariableId& variableId)
    {
        AZStd::vector<NodeIdPair> variableNodes;

        if (variableId.IsValid())
        {
            AZStd::unordered_set< ScriptCanvas::VariableId > variableIds = { variableId };

            for (auto nodePairMapping : GetNodeMapping())
            {
                if (nodePairMapping.second->ContainsReferencesToVariables(variableIds))
                {
                    NodeIdPair nodeIdPair;
                    nodeIdPair.m_scriptCanvasId = nodePairMapping.first;
                    SceneMemberMappingRequestBus::EventResult(nodeIdPair.m_graphCanvasId, nodePairMapping.first, &SceneMemberMappingRequests::GetGraphCanvasEntityId);
                    variableNodes.push_back(nodeIdPair);
                }
            }
        }

        return variableNodes;
    }

    void Graph::QueueVersionUpdate(const AZ::EntityId& graphCanvasNodeId)
    {
        bool queueUpdate = m_queuedConvertingNodes.empty();
        auto insertResult = m_queuedConvertingNodes.insert(graphCanvasNodeId);

        if (insertResult.second && queueUpdate)
        {
            m_allowVersionUpdate = false;
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    bool Graph::CanExposeEndpoint(const GraphCanvas::Endpoint& endpoint)
    {
        bool isEnabled = false;

        GraphCanvas::SlotType slotType;
        GraphCanvas::SlotRequestBus::EventResult(slotType, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetSlotType);

        if (slotType == GraphCanvas::SlotTypes::DataSlot)
        {
            GraphCanvas::DataSlotType dataSlotType = GraphCanvas::DataSlotType::Unknown;
            GraphCanvas::DataSlotRequestBus::EventResult(dataSlotType, endpoint.GetSlotId(), &GraphCanvas::DataSlotRequests::GetDataSlotType);

            if (dataSlotType != GraphCanvas::DataSlotType::Value)
            {
                isEnabled = false;
            }

            bool hasConnections = false;
            GraphCanvas::SlotRequestBus::EventResult(hasConnections, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::HasConnections);

            if (hasConnections)
            {
                isEnabled = false;
            }

            ScriptCanvas::Endpoint scEndpoint = ConvertToScriptCanvasEndpoint(endpoint);
            ScriptCanvas::Slot* slot = FindSlot(scEndpoint);

            // If we don't have a slot it likely means this is a remapped visual slot.
            // So we don't want to perform many operations on it.
            if (slot)
            {
                ScriptCanvas::Data::Type dataType = slot->GetDataType();

                bool isValidVariableType = false;
                VariablePaletteRequestBus::BroadcastResult(isValidVariableType, &VariablePaletteRequests::IsValidVariableType, dataType);

                if (!isValidVariableType)
                {
                    isEnabled = false;
                }
            }
            else
            {
                isEnabled = false;
            }
        }
        else
        {
            isEnabled = true;
        }

        bool isNodeling = false;
        NodeDescriptorRequestBus::EventResult(isNodeling, endpoint.GetNodeId(), &NodeDescriptorRequests::IsType, NodeDescriptorType::FunctionDefinitionNode);        

        return isEnabled && !isNodeling;
    }

    ScriptCanvas::Endpoint Graph::ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const
    {
        AZStd::any* userData = nullptr;

        ScriptCanvas::Endpoint scriptCanvasEndpoint;

        GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetUserData);
        ScriptCanvas::SlotId scSourceSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();
        userData = nullptr;

        AZ::EntityId scriptCanvasNodeId = ConvertToScriptCanvasNodeId(endpoint.GetNodeId());

        scriptCanvasEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, scSourceSlotId);

        return scriptCanvasEndpoint;
    }

    GraphCanvas::Endpoint Graph::ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint) const
    {
        GraphCanvas::Endpoint graphCanvasEndpoint;

        SlotMappingRequestBus::EventResult(graphCanvasEndpoint.m_slotId, endpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, endpoint.GetSlotId());
        GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint.m_nodeId, graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetNode);

        return graphCanvasEndpoint;
    }

    void Graph::OnSaveDataDirtied(const AZ::EntityId& savedElement)
    {
        // The EbusHandlerEvent's are a visual only representation of alternative data, and should not be saved.
        if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(savedElement) != nullptr
            || m_ignoreSaveRequests)
        {
            return;
        }

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, savedElement, &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            const AZ::EntityId* scriptCanvasNodeId = AZStd::any_cast<AZ::EntityId>(userData);
            GraphCanvas::EntitySaveDataContainer* container = nullptr;

            auto mapIter = m_graphCanvasSaveData.find((*scriptCanvasNodeId));

            if (mapIter == m_graphCanvasSaveData.end())
            {
                container = aznew GraphCanvas::EntitySaveDataContainer();
                m_graphCanvasSaveData[(*scriptCanvasNodeId)] = container;
            }
            else
            {
                container = mapIter->second;
            }

            GraphCanvas::EntitySaveDataRequestBus::Event(savedElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, (*container));
        }
        else if (savedElement == GetGraphCanvasGraphId())
        {
            GraphCanvas::EntitySaveDataContainer* container = nullptr;
            auto mapIter = m_graphCanvasSaveData.find(GetEntityId());

            if (mapIter == m_graphCanvasSaveData.end())
            {
                container = aznew GraphCanvas::EntitySaveDataContainer();
                m_graphCanvasSaveData[GetEntityId()] = container;
            }
            else
            {
                container = mapIter->second;
            }

            GraphCanvas::EntitySaveDataRequestBus::Event(savedElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, (*container));

            m_statisticsHelper.PopulateStatisticData(this);
        }
    }

    bool Graph::NeedsSaveConversion() const
    {
        return !m_saveFormatConverted;
    }

    void Graph::ConvertSaveFormat()
    {
        if (!m_saveFormatConverted)
        {
            // Bit of a work around for not being able to clean this up in the actual save.
            m_saveFormatConverted = true;

            // SceneComponent
            for (const AZ::Uuid& componentType : {
                AZ::Uuid("{3F71486C-3D51-431F-B904-DA070C7A0238}"), // GraphCanvas::SceneComponent
                AZ::Uuid("{486B009F-632B-44F6-81C2-3838746190AE}"), // ColorPaletteManagerComponent
                AZ::Uuid("{A8F08DEA-0F42-4236-9E1E-B93C964B113F}"), // BookmarkManagerComponent
                AZ::Uuid("{34B81206-2C69-4886-945B-4A9ECC0FDAEE}")  // StyleSheet
            }
                )
            {
                AZ::Component* component = GetEntity()->FindComponent(componentType);

                if (component)
                {
                    if (GetEntity()->RemoveComponent(component))
                    {
                        delete component;
                    }
                }
            }
        }
    }

    void Graph::ConstructSaveData()
    {
        // Save out the SceneData
        //
        // For this one all of the GraphCanvas information lives on the same entity.
        // So we need to use that key to look up everything
        {
            OnSaveDataDirtied(GetGraphCanvasGraphId());
        }
        
        AZStd::vector< AZ::EntityId > graphCanvasNodes;
        GraphCanvas::SceneRequestBus::EventResult(graphCanvasNodes, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetNodes);

        for (const AZ::EntityId& graphCanvasNode : graphCanvasNodes)
        {
            OnSaveDataDirtied(graphCanvasNode);
        }
    }

    void Graph::OnToastInteraction()
    {
        const AzToolsFramework::ToastId* toastId = AzToolsFramework::ToastNotificationBus::GetCurrentBusId();

        if (toastId)
        {
            NodeIdPair pair;
            pair.m_scriptCanvasId = m_toastNodeIds[(*toastId)];

            UnregisterToast((*toastId));

            SceneMemberMappingRequestBus::EventResult(pair.m_graphCanvasId, pair.m_scriptCanvasId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);            

            AZStd::vector<AZ::EntityId> focusElements = { pair.m_graphCanvasId };

            m_focusHelper.Clear();
            m_focusHelper.SetNodes(focusElements);

            m_focusHelper.CycleToNextNode();

            AZStd::vector< NodeIdPair > highlightPair = { pair };
            HighlightNodes(highlightPair);
        }
    }

    void Graph::OnToastDismissed()
    {
        const AzToolsFramework::ToastId* toastId = AzToolsFramework::ToastNotificationBus::GetCurrentBusId();

        if (toastId)
        {
            UnregisterToast((*toastId));
        }
    }

    void Graph::OnUndoRedoEnd()
    {
        for (const auto& nodePair : GetNodeMapping())
        {
            nodePair.second->SignalDeserialized();
        }
    }

    void Graph::ReportError(const ScriptCanvas::Node& node, const AZStd::string& errorSource, const AZStd::string& errorMessage)
    {
        AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Error, errorSource.c_str(), errorMessage.c_str());

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetViewId);

        AzToolsFramework::ToastId toastId;
        GraphCanvas::ViewRequestBus::EventResult(toastId, viewId, &GraphCanvas::ViewRequests::ShowToastNotification, toastConfiguration);        

        AzToolsFramework::ToastNotificationBus::MultiHandler::BusConnect(toastId);
        m_toastNodeIds[toastId] = node.GetEntityId();
    }

    void Graph::UnregisterToast(const AzToolsFramework::ToastId& toastId)
    {
        AzToolsFramework::ToastNotificationBus::MultiHandler::BusDisconnect(toastId);
        m_toastNodeIds.erase(toastId);
    }

    void Graph::DisplayUpdateToast()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetViewId);

        if (viewId.IsValid() && !m_updateStrings.empty())
        {
            bool isVisible = false;

            GraphCanvas::ViewRequestBus::EventResult(isVisible, viewId, &GraphCanvas::ViewRequests::IsShowing);

            if (isVisible)
            {
                AZStd::string displayString;

                for (const auto& updateData : m_updateStrings)
                {
                    if (!displayString.empty())
                    {
                        displayString.append("\n");
                    }

                    displayString.append("- ");
                    displayString.append(updateData);
                }

                m_updateStrings.clear();

                AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Information, "Nodes Updates", displayString.c_str());
                GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowToastNotification, toastConfiguration);
            }
        }
    }

    const GraphStatisticsHelper& Graph::GetNodeUsageStatistics() const
    {
        return m_statisticsHelper;
    }

    void Graph::CreateGraphCanvasScene()
    {
        if (!m_saveFormatConverted)
        {
            GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);
            GraphCanvas::GraphModelRequestBus::Handler::BusConnect(graphCanvasGraphId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetEditorId, ScriptCanvasEditor::AssetEditorId);

            AZStd::any* userData = nullptr;
            GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserData);

            if (userData)
            {
                (*userData) = GetScriptCanvasId();
            }
        }
        else if (m_graphCanvasSceneEntity == nullptr)
        {
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(m_graphCanvasSceneEntity, &GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);

            if (m_graphCanvasSceneEntity == nullptr)
            {
                return;
            }

            AZ::EntityId graphCanvasGraphId = GetGraphCanvasGraphId();
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetEditorId, ScriptCanvasEditor::AssetEditorId);

            DisplayGraphCanvasScene();

            AZStd::any* userData = nullptr;
            GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserData);

            if (userData)
            {
                (*userData) = GetScriptCanvasId();
            }
        }

        m_focusHelper.SetActiveGraph(GetGraphCanvasGraphId());
    }

    bool Graph::UpgradeGraph(SourceHandle& asset, UpgradeRequest request, bool isVerbose)
    {
        m_upgradeSM.SetAsset(asset);
        m_upgradeSM.SetVerbose(isVerbose);

        if (request == UpgradeRequest::Forced || !GetVersion().IsLatest())
        {
            m_upgradeSM.Run(Start::StateID());
            return true;
        }
        else
        {
            m_upgradeSM.Run(Skip::StateID());
            return false;
        }
    }

    void Graph::ConnectGraphCanvasBuses()
    {
        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        GraphCanvas::GraphModelRequestBus::Handler::BusConnect(graphCanvasGraphId);
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);
    }

    void Graph::DisconnectGraphCanvasBuses()
    {
        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

    }

    void Graph::OnSystemTick()
    {
        if (!m_allowVersionUpdate)
        {
            m_allowVersionUpdate = true;
        }
        else
        {
            m_allowVersionUpdate = false;
            AZ::SystemTickBus::Handler::BusDisconnect();

            HandleQueuedUpdates();
        }
    }

    void Graph::DisplayGraphCanvasScene()
    {
        m_variableDataModel.Activate(GetScriptCanvasId());

        RequestPushPreventUndoStateUpdate();

        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > scriptCanvasToGraphCanvasMapping;

        bool graphNeedsDirtying = !GetVersion().IsLatest();
        {
            QScopedValueRollback<bool> ignoreRequests(m_ignoreSaveRequests, true);

            GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

            GraphCanvas::GraphModelRequestBus::Handler::BusConnect(graphCanvasGraphId);
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadStart);

            auto saveDataIter = m_graphCanvasSaveData.find(GetEntityId());

            if (saveDataIter != m_graphCanvasSaveData.end())
            {
                GraphCanvas::EntitySaveDataRequestBus::Event(graphCanvasGraphId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter->second));
            }

            ScriptCanvas::NodeIdList nodeList = GetNodes();
            
            AZStd::unordered_set<ScriptCanvas::Node*> outOfDateNodes;
            AZStd::unordered_set<AZ::EntityId> deletedNodes;
            AZStd::unordered_set<AZ::EntityId> assetSanitizationSet;
            AZStd::unordered_set<ScriptCanvas::Node*> sanityCheckRequiredNodes;

            ScriptCanvas::GraphUpdateSlotReport graphUpdateSlotReport;

            for (const AZ::EntityId& scriptCanvasNodeId : nodeList)
            {
                assetSanitizationSet.insert(scriptCanvasNodeId);

                ScriptCanvas::Node* scriptCanvasNode = FindNode(scriptCanvasNodeId);

                if (scriptCanvasNode)
                {
                     if (scriptCanvasNode->IsDeprecated() && !g_disableDeprecatedNodeUpdates)
                    {
                        ScriptCanvas::NodeConfiguration nodeConfig = scriptCanvasNode->GetReplacementNodeConfiguration();
                        if (nodeConfig.IsValid())
                        {
                            ScriptCanvas::NodeUpdateSlotReport nodeUpdateSlotReport;
                            auto nodeOutcome = ReplaceNodeByConfig(scriptCanvasNode, nodeConfig, nodeUpdateSlotReport);

                            if (nodeOutcome.IsSuccess())
                            {
                                graphNeedsDirtying = true;
                                scriptCanvasNode = nodeOutcome.GetValue();
                                m_updateStrings.insert(AZStd::string::format("Replaced node (%s)", scriptCanvasNode->GetNodeName().c_str()));
                                ScriptCanvas::MergeUpdateSlotReport(scriptCanvasNodeId, graphUpdateSlotReport, nodeUpdateSlotReport);
                            }
                        }
                    }

                    AZ::EntityId graphCanvasNodeId = Nodes::DisplayScriptCanvasNode(graphCanvasGraphId, scriptCanvasNode);
                    scriptCanvasToGraphCanvasMapping[scriptCanvasNodeId] = graphCanvasNodeId;

                    auto saveDataIter2 = m_graphCanvasSaveData.find(scriptCanvasNodeId);
                    if (saveDataIter2 != m_graphCanvasSaveData.end())
                    {
                        GraphCanvas::EntitySaveDataRequestBus::Event(graphCanvasNodeId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter2->second));
                    }

                    AZ::Vector2 position;
                    GraphCanvas::GeometryRequestBus::EventResult(position, graphCanvasNodeId, &GraphCanvas::GeometryRequests::GetPosition);

                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasNodeId, position, false);

                    // If the node is deprecated, we want to stomp whatever style it had saved and apply the deprecated style
                    if (scriptCanvasNode->IsDeprecated())
                    {
                        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "DeprecatedNodeTitlePalette");
                    }

                    if (scriptCanvasNode->IsOutOfDate(GetVersion()))
                    {
                        OnVersionConversionBegin((*scriptCanvasNode));
                        outOfDateNodes.emplace(scriptCanvasNode);
                    }

                    if (scriptCanvasNode->IsSanityCheckRequired())
                    {
                        graphNeedsDirtying = true;
                        sanityCheckRequiredNodes.insert(scriptCanvasNode);
                    }
                }
            }

            if (!graphUpdateSlotReport.IsEmpty())
            {
                // currently, it is expected that there are no deleted old slots, those need manual correction
                AZ_Error("ScriptCanvas", graphUpdateSlotReport.m_deletedOldSlots.empty(), "Graph upgrade path: If old slots are deleted, manual upgrading is required");
                UpdateConnectionStatus(*this, graphUpdateSlotReport);
            }

            AZStd::unordered_set<AZ::EntityId> graphCanvasNodesToDelete;

            for (auto scriptCanvasNode : outOfDateNodes)
            {
                auto graphCanvasNodeId = scriptCanvasToGraphCanvasMapping[scriptCanvasNode->GetEntityId()];
                ScriptCanvas::UpdateResult updateResult = scriptCanvasNode->UpdateNode();
                OnVersionConversionEnd((*scriptCanvasNode));

                switch (updateResult)
                {
                case ScriptCanvas::UpdateResult::DeleteNode:
                {
                    graphNeedsDirtying = true;
                    deletedNodes.insert(scriptCanvasNode->GetEntityId());
                    graphCanvasNodesToDelete.insert(graphCanvasNodeId);
                    break;
                }
                default:
                {
                    graphNeedsDirtying = true;
                    break;
                }
                }
            }

            if (!graphCanvasNodesToDelete.empty())
            {
                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, graphCanvasNodesToDelete);
            }

            AZStd::vector< AZ::EntityId > connectionIds = GetConnections();

            for (const AZ::EntityId& connectionId : connectionIds)
            {
                ScriptCanvas::Endpoint scriptCanvasSourceEndpoint;
                ScriptCanvas::Endpoint scriptCanvasTargetEndpoint;

                ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasSourceEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetSourceEndpoint);
                ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasTargetEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetTargetEndpoint);

                AZ::EntityId graphCanvasSourceNode;

                auto scriptCanvasIter = scriptCanvasToGraphCanvasMapping.find(scriptCanvasSourceEndpoint.GetNodeId());

                if (scriptCanvasIter != scriptCanvasToGraphCanvasMapping.end())
                {
                    graphCanvasSourceNode = scriptCanvasIter->second;
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
                }

                AZ::EntityId graphCanvasSourceSlotId;
                SlotMappingRequestBus::EventResult(graphCanvasSourceSlotId, graphCanvasSourceNode, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSourceEndpoint.GetSlotId());

                if (!graphCanvasSourceSlotId.IsValid())
                {
                    // For the EBusHandler's I need to remap these to a different visual node.
                    // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                    if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasSourceNode) != nullptr)
                    {
                        GraphCanvas::Endpoint graphCanvasEventEndpoint;
                        EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasEventEndpoint, graphCanvasSourceNode, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasSourceEndpoint.GetSlotId());

                        graphCanvasSourceSlotId = graphCanvasEventEndpoint.GetSlotId();
                    }

                    if (!graphCanvasSourceSlotId.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", deletedNodes.count(scriptCanvasSourceEndpoint.GetNodeId()) > 0, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasSourceEndpoint.GetNodeId().ToString().c_str());
                        DisconnectById(connectionId);
                        continue;
                    }
                }

                GraphCanvas::Endpoint graphCanvasTargetEndpoint;

                scriptCanvasIter = scriptCanvasToGraphCanvasMapping.find(scriptCanvasTargetEndpoint.GetNodeId());

                if (scriptCanvasIter != scriptCanvasToGraphCanvasMapping.end())
                {
                    graphCanvasTargetEndpoint.m_nodeId = scriptCanvasIter->second;
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
                }

                SlotMappingRequestBus::EventResult(graphCanvasTargetEndpoint.m_slotId, graphCanvasTargetEndpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasTargetEndpoint.GetSlotId());

                if (!graphCanvasTargetEndpoint.IsValid())
                {
                    // For the EBusHandler's I need to remap these to a different visual node.
                    // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                    if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasTargetEndpoint.GetNodeId()) != nullptr)
                    {
                        EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasTargetEndpoint, graphCanvasTargetEndpoint.GetNodeId(), &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasTargetEndpoint.GetSlotId());
                    }

                    if (!graphCanvasTargetEndpoint.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", deletedNodes.count(scriptCanvasTargetEndpoint.GetNodeId()) > 0, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasTargetEndpoint.GetNodeId().ToString().c_str());
                        DisconnectById(connectionId);
                        continue;
                    }
                }

                AZ::EntityId graphCanvasConnectionId;
                GraphCanvas::SlotRequestBus::EventResult(graphCanvasConnectionId, graphCanvasSourceSlotId, &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, graphCanvasTargetEndpoint);

                if (graphCanvasConnectionId.IsValid())
                {
                    AZStd::any* userData = nullptr;
                    GraphCanvas::ConnectionRequestBus::EventResult(userData, graphCanvasConnectionId, &GraphCanvas::ConnectionRequests::GetUserData);

                    if (userData)
                    {
                        (*userData) = connectionId;

                        SceneMemberMappingConfigurationRequestBus::Event(graphCanvasConnectionId, &SceneMemberMappingConfigurationRequests::ConfigureMapping, connectionId);
                    }
                }
            }

            // Fix up leaked data elements
            auto mapIter = m_graphCanvasSaveData.begin();

            while (mapIter != m_graphCanvasSaveData.end())
            {
                // Deleted using the wrong id, which orphaned the SaveData. For now we want to go through and sanitize our save data to avoid keeping around a bunch
                // of old save data for no reason.
                //
                // Need to bypass our internal save data for graph canvas information
                if (scriptCanvasToGraphCanvasMapping.find(mapIter->first) == scriptCanvasToGraphCanvasMapping.end()
                    && mapIter->first != GetEntityId())
                {
                    delete mapIter->second;
                    mapIter = m_graphCanvasSaveData.erase(mapIter);
                }
                else
                {
                    ++mapIter;
                }
            }

            auto currentIter = GetGraphData()->m_scriptEventAssets.begin();

            while (currentIter != GetGraphData()->m_scriptEventAssets.end())
            {
                if (assetSanitizationSet.find(currentIter->first) == assetSanitizationSet.end())
                {
                    currentIter->second = {};
                    currentIter = GetGraphData()->m_scriptEventAssets.erase(currentIter);
                    graphNeedsDirtying = true;
                }
                else
                {
                    ++currentIter;
                }
            }

            for (auto node : sanityCheckRequiredNodes)
            {
                if (node)
                {
                    node->SanityCheckDynamicDisplay();
                }
            }

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadEnd);
            EditorGraphNotificationBus::Event(GetScriptCanvasId(), &EditorGraphNotifications::OnGraphCanvasSceneDisplayed);
        }

        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::ProcessEnableDisableQueue);

        if (m_graphCanvasSaveVersion != GraphCanvas::EntitySaveDataContainer::CurrentVersion)
        {
            for (auto saveDataPair : m_graphCanvasSaveData)
            {
                auto graphCanvasIter = scriptCanvasToGraphCanvasMapping.find(saveDataPair.first);
                OnSaveDataDirtied(graphCanvasIter->second);
            }

            m_graphCanvasSaveVersion = GraphCanvas::EntitySaveDataContainer::CurrentVersion;
            graphNeedsDirtying = true;
        }

        RequestPopPreventUndoStateUpdate();

        if (graphNeedsDirtying)
        {
            SignalDirty();
        }

        MarkVersion();
    }

    void Graph::OnGraphCanvasSceneVisible()
    {
        DisplayUpdateToast();
    }

    AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > Graph::GetGraphCanvasSaveData()
    {
        return m_graphCanvasSaveData;
    }

    void Graph::UpdateGraphCanvasSaveData(const AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* >& saveData)
    {
        QScopedValueRollback<bool> ignoreRequests(m_ignoreSaveRequests, true);

        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect(graphCanvasGraphId);
        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect(graphCanvasGraphId);

        for (auto& entry : m_graphCanvasSaveData)
        {
            delete entry.second;
        }

        m_graphCanvasSaveData = saveData;

        DisplayGraphCanvasScene();
    }

    void Graph::ClearGraphCanvasScene()
    {
        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        RequestPushPreventUndoStateUpdate();

        // Wipe out all of the Graph Canvas Visuals
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearScene);

        RequestPopPreventUndoStateUpdate();
    }    
}
