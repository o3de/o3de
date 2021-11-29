/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Bus/GraphBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Utils/VersioningUtils.h>

#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVariableDataInterface.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/Components/EditorUtils.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

#include <GraphCanvas/Widgets/NodePropertyBus.h>

#include <Editor/Include/ScriptCanvas/Components/GraphUpgrade.h>

namespace ScriptCanvas
{
    struct NodeConfiguration;
    struct NodeUpdateSlotReport;
}

namespace ScriptCanvasEditor
{
    //! EditorGraph is the editor version of the ScriptCanvas::Graph component that is activated when executing the script canvas engine
    class Graph
        : public ScriptCanvas::Graph
        , private NodeCreationNotificationBus::Handler
        , private SceneCounterRequestBus::Handler
        , private EditorGraphRequestBus::Handler
        , private GraphCanvas::GraphModelRequestBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , private GraphItemCommandNotificationBus::Handler
        , private AzToolsFramework::ToastNotificationBus::MultiHandler
        , private GeneralEditorNotificationBus::Handler
        , private AZ::SystemTickBus::Handler
    {
    private:

        friend class Start;
        friend class PreRequisites;
        friend class PreventUndo;
        friend class CollectData;
        friend class ReplaceDeprecatedNodes;
        friend class RemapConnections;
        friend class VerifySaveDataVersion;
        friend class SanityChecks;
        friend class UpgradeScriptEvents;
        friend class UpdateOutOfDateNodes;
        friend class BuildGraphCanvasMapping;
        friend class FixLeakedData;
        friend class RestoreUndo;
        friend class DisplayReport;
        friend class Finalize;

        typedef AZStd::unordered_map< AZ::EntityId, AZ::EntityId > WrappedNodeGroupingMap;

        static void ConvertToGetVariableNode(Graph* graph, ScriptCanvas::VariableId variableId, const AZ::EntityId& nodeId, AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& setVariableRemapping);

        struct CRCCache
        {
            AZ_TYPE_INFO(CRCCache, "{59798D92-94AD-4A08-8F38-D5975B0DC33B}");

            CRCCache()
                : m_cacheCount(0)
            {
            }

            CRCCache(const AZStd::string& cacheString)
                : m_cacheValue(cacheString)
                , m_cacheCount(1)
            {

            }

            AZStd::string m_cacheValue;
            int m_cacheCount;
        };

    public:
        AZ_COMPONENT(Graph, "{4D755CA9-AB92-462C-B24F-0B3376F19967}", ScriptCanvas::Graph);

        static void Reflect(AZ::ReflectContext* context);

        Graph(const ScriptCanvas::ScriptCanvasId& scriptCanvasId = AZ::Entity::MakeId())
            : ScriptCanvas::Graph(scriptCanvasId)
            , m_variableCounter(0)
            , m_graphCanvasSceneEntity(nullptr)
            , m_ignoreSaveRequests(false)
            , m_graphCanvasSaveVersion(GraphCanvas::EntitySaveDataContainer::CurrentVersion)
            , m_upgradeSM(this)
        {}

        ~Graph() override;

        void Activate() override;
        void Deactivate() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ScriptCanvas::Graph::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            ScriptCanvas::Graph::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        // SceneCounterRequestBus
        AZ::u32 GetNewVariableCounter() override;
        void ReleaseVariableCounter(AZ::u32 variableCounter) override;
        ////

        // RuntimeBus
        AZ::Data::AssetId GetAssetId() const override { return m_assetId; }
        ////

        // GraphCanvas::GraphModelRequestBus
        void RequestUndoPoint() override;

        void RequestPushPreventUndoStateUpdate() override;
        void RequestPopPreventUndoStateUpdate() override;

        void TriggerUndo() override;
        void TriggerRedo() override;

        void EnableNodes(const AZStd::unordered_set< GraphCanvas::NodeId >& nodeIds) override;
        void DisableNodes(const AZStd::unordered_set< GraphCanvas::NodeId >& nodeIds) override;

        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;

        void DisconnectConnection(const GraphCanvas::ConnectionId& connectionId) override;
        bool CreateConnection(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;

        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;

        AZStd::string GetDataTypeString(const AZ::Uuid& typeId) override;

        void OnRemoveUnusedNodes() override;
        void OnRemoveUnusedElements() override;

        bool AllowReset(const GraphCanvas::Endpoint& endpoint) const override;

        void ResetSlotToDefaultValue(const GraphCanvas::Endpoint& endpoint) override;
        void ResetReference(const GraphCanvas::Endpoint& endpoint) override;
        void ResetProperty(const GraphCanvas::NodeId& nodeId, const AZ::Crc32& propertyId) override;

        void RemoveSlot(const GraphCanvas::Endpoint& endpoint) override;
        bool IsSlotRemovable(const GraphCanvas::Endpoint& endpoint) const override;

        bool ConvertSlotToReference(const GraphCanvas::Endpoint& endpoint) override;
        bool CanConvertSlotToReference(const GraphCanvas::Endpoint& endpoint) override;
        GraphCanvas::CanHandleMimeEventOutcome CanHandleReferenceMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData) override;
        bool HandleReferenceMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData) override;
        bool CanPromoteToVariable(const GraphCanvas::Endpoint& endpoint) const override;
        bool PromoteToVariableAction(const GraphCanvas::Endpoint& endpoint) override;
        bool SynchronizeReferences(const GraphCanvas::Endpoint& sourceEndpoint, const GraphCanvas::Endpoint& targetEndpoint) override;

        bool ConvertSlotToValue(const GraphCanvas::Endpoint& endpoint) override;
        bool CanConvertSlotToValue(const GraphCanvas::Endpoint& endpoint) override;
        GraphCanvas::CanHandleMimeEventOutcome CanHandleValueMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData) override;
        bool HandleValueMimeEvent(const GraphCanvas::Endpoint& endpoint, const QMimeData* mimeData) override;

        GraphCanvas::SlotId RequestExtension(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId, GraphModelRequests::ExtensionRequestReason) override;
        void ExtensionCancelled(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId) override;
        void FinalizeExtension(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId) override;

        bool ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const override;

        void AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        void RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        ////

        // SceneNotificationBus
        void OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnPreNodeDeleted(const AZ::EntityId& nodeId) override;
        void OnPreConnectionDeleted(const AZ::EntityId& nodeId) override;

        void OnUnknownPaste(const QPointF& scenePos) override;

        void OnSelectionChanged() override;

        void PostDeletionEvent() override;
        void PostCreationEvent() override;
        void OnPasteBegin() override;
        void OnPasteEnd() override;

        void OnViewRegistered() override;
        /////////////////////////////////////////////////////////////////////////////////////////////

        //! NodeCreationNotifications
        void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) override;
        ///////////////////////////

        // EditorGraphRequestBus
        void SetAssetId(const AZ::Data::AssetId& assetId) override { m_assetId = assetId; }

        void CreateGraphCanvasScene() override;
        void ClearGraphCanvasScene() override;
        void DisplayGraphCanvasScene() override;

        /////
        EditorGraphUpgradeMachine m_upgradeSM;
        enum UpgradeRequest
        {
            IfOutOfDate,
            Forced
        };
        bool UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset, UpgradeRequest request, bool isVerbose = true);
        void ConnectGraphCanvasBuses();
        void DisconnectGraphCanvasBuses();
        ///////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        void OnGraphCanvasSceneVisible() override;

        GraphCanvas::GraphId GetGraphCanvasGraphId() const override;

        AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > GetGraphCanvasSaveData() override;
        void UpdateGraphCanvasSaveData(const AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* >& saveData) override;

        NodeIdPair CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position) override;

        void AddCrcCache(const AZ::Crc32& crcValue, const AZStd::string& cacheString) override;
        void RemoveCrcCache(const AZ::Crc32& crcValue) override;
        AZStd::string DecodeCrc(const AZ::Crc32& crcValue) override;

        void ClearHighlights() override;
        void HighlightMembersFromTreeItem(const GraphCanvas::GraphCanvasTreeItem* treeItem) override;
        void HighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds) override;
        void HighlightNodes(const AZStd::vector<NodeIdPair>& nodes) override;

        AZStd::vector<NodeIdPair> GetNodesOfType(const ScriptCanvas::NodeTypeIdentifier&) override;
        AZStd::vector<NodeIdPair> GetVariableNodes(const ScriptCanvas::VariableId&) override;

        void RemoveUnusedVariables() override;

        bool CanConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId) override;
        bool ConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId) override;
        bool ConvertReferenceToVariableNode(const GraphCanvas::Endpoint& endpoint) override;

        void QueueVersionUpdate(const AZ::EntityId& graphCanvasNodeId) override;

        bool CanExposeEndpoint(const GraphCanvas::Endpoint& endpoint) override;

        ScriptCanvas::Endpoint ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const override;
        GraphCanvas::Endpoint ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint) const override;
        ////

        bool OnVersionConversionBegin(ScriptCanvas::Node& node);
        void OnVersionConversionEnd(ScriptCanvas::Node& node);

        // EntitySaveDataGraphActionBus
        void OnSaveDataDirtied(const AZ::EntityId& savedElement) override;
        ////

        // Save Information Conversion
        bool NeedsSaveConversion() const;
        void ConvertSaveFormat();

        void ConstructSaveData();
        ////

        // ToastNotifications
        void OnToastInteraction() override;
        void OnToastDismissed() override;
        ////

        // GeneralEditorNotificationBus
        void OnUndoRedoEnd() override;
        ////

        void SetAssetType(AZ::Data::AssetType);

        void ReportError(const ScriptCanvas::Node& node, const AZStd::string& errorSource, const AZStd::string& errorMessage) override;

        const GraphStatisticsHelper& GetNodeUsageStatistics() const;

        // Finds and returns all nodes within the graph that are of the specified type
        template <typename NodeType>
        AZStd::vector<const NodeType*> GetNodesOfType() const
        {
            AZStd::vector<const NodeType*> nodes;
            for (auto& nodeRef : m_graphData.m_nodes)
            {
                const NodeType* node = nodeRef->FindComponent<NodeType>();
                if (node)
                {
                    nodes.push_back(node);
                }
            }
            return nodes;
        }

    protected:
        void PostRestore(const UndoData& restoredData) override;

        void UnregisterToast(const AzToolsFramework::ToastId& toastId);

        Graph(const Graph&) = delete;

        void DisplayUpdateToast();

        AZ::EntityId          ConvertToScriptCanvasNodeId(const GraphCanvas::NodeId& nodeId) const;

        GraphCanvas::NodePropertyDisplay* CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const;

        void SignalDirty();

        void HighlightNodesByType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier);
        void HighlightEBusNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId);
        void HighlightScriptEventNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId);
        void HighlightScriptCanvasEntity(const AZ::EntityId& scriptCanvasId);

        AZ::EntityId FindGraphCanvasSlotId(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId);
        bool ConfigureConnectionUserData(const ScriptCanvas::Endpoint& sourceEndpoint, const ScriptCanvas::Endpoint& targetEndpoint, GraphCanvas::ConnectionId connectionId);

        void HandleQueuedUpdates();
        bool IsNodeVersionConverting(const AZ::EntityId& graphCanvasNodeId) const;

        AZStd::unordered_map< AzToolsFramework::ToastId, AZ::EntityId > m_toastNodeIds;

        // Function Definition Node Extension
        void HandleFunctionDefinitionExtension(ScriptCanvas::Node* node, GraphCanvas::SlotId graphCanvasSlotId, const GraphCanvas::NodeId& nodeId);

        //// Version Update code
        AZ::Outcome<ScriptCanvas::Node*> ReplaceNodeByConfig(ScriptCanvas::Node*, const ScriptCanvas::NodeConfiguration&, ScriptCanvas::NodeUpdateSlotReport& nodeUpdateSlotReport);
        bool SanityCheckNodeReplacement(ScriptCanvas::Node*, ScriptCanvas::Node*, ScriptCanvas::NodeUpdateSlotReport& nodeUpdateSlotReport);

        bool m_allowVersionUpdate = false;
        AZStd::unordered_set< AZ::EntityId > m_queuedConvertingNodes;
        AZStd::unordered_set< AZ::EntityId > m_convertingNodes;
        AZStd::unordered_multimap< AZ::EntityId, ScriptCanvas::SlotId > m_versionedSlots;
        AZStd::unordered_set< AZStd::string > m_updateStrings;

        AZ::u32 m_variableCounter;
        AZ::EntityId m_wrapperNodeDropTarget;

        VariableComboBoxDataModel m_variableDataModel;

        WrappedNodeGroupingMap m_wrappedNodeGroupings;
        AZStd::vector< AZ::EntityId > m_lastGraphCanvasCreationGroup;

        AZ::Entity* m_graphCanvasSceneEntity;

        GraphCanvas::EntitySaveDataContainer::VersionInformation m_graphCanvasSaveVersion;
        AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > m_graphCanvasSaveData;

        AZStd::unordered_map< AZ::Crc32, CRCCache > m_crcCacheMap;

        AZStd::unordered_set< GraphCanvas::GraphicsEffectId > m_highlights;

        GraphCanvas::NodeFocusCyclingHelper m_focusHelper;
        GraphStatisticsHelper m_statisticsHelper;

        bool m_ignoreSaveRequests;

        //! Defaults to true to signal that this graph does not have the GraphCanvas stuff intermingled
        bool m_saveFormatConverted = true;

        AZ::Data::AssetId m_assetId;
    };
}
