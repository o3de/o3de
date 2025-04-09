/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/EntitySaveData.h>

#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <AzCore/Interface/Interface.h>

#include <ScriptCanvas/Core/Core.h>

namespace GraphCanvas
{
    class GraphCanvasTreeItem;
}

namespace ScriptCanvasEditor
{
    class EditorScriptCanvasComponentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetAssetId(const SourceHandle& assetId) = 0;
        virtual bool HasAssetId() const = 0;
    };

    using EditorScriptCanvasComponentRequestBus = AZ::EBus<EditorScriptCanvasComponentRequests>;

    class EditorGraphRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        virtual void CreateGraphCanvasScene() = 0;
        virtual void ClearGraphCanvasScene() = 0;
        virtual GraphCanvas::GraphId GetGraphCanvasGraphId() const = 0;

        virtual void DisplayGraphCanvasScene() = 0;

        virtual void OnGraphCanvasSceneVisible() = 0;

        virtual void UpdateGraphCanvasSaveData(const AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* >& saveData) = 0;
        virtual AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > GetGraphCanvasSaveData() = 0;

        virtual NodeIdPair CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position) = 0;

        virtual void AddCrcCache(const AZ::Crc32& crcValue, const AZStd::string& cacheString) = 0;
        virtual void RemoveCrcCache(const AZ::Crc32& crcValue) = 0;
        virtual AZStd::string DecodeCrc(const AZ::Crc32& crcValue) = 0;

        virtual void ClearHighlights() = 0;
        virtual void HighlightMembersFromTreeItem(const GraphCanvas::GraphCanvasTreeItem* treeItem) = 0;
        virtual void HighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds) = 0;
        virtual void HighlightNodes(const AZStd::vector<NodeIdPair>& nodes) = 0;

        virtual AZStd::vector<NodeIdPair> GetNodesOfType(const ScriptCanvas::NodeTypeIdentifier&) = 0;
        virtual AZStd::vector<NodeIdPair> GetVariableNodes(const ScriptCanvas::VariableId&) = 0;

        virtual void RemoveUnusedVariables() = 0;

        virtual bool CanConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId) = 0;
        virtual bool ConvertVariableNodeToReference(const GraphCanvas::NodeId& nodeId) = 0;
        virtual bool ConvertReferenceToVariableNode(const GraphCanvas::Endpoint& endpoint) = 0;

        virtual void QueueVersionUpdate(const AZ::EntityId& graphCanvasNodeId) = 0;
        virtual bool CanExposeEndpoint(const GraphCanvas::Endpoint& endpoint) = 0;

        virtual ScriptCanvas::Endpoint ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoinnt) const = 0;
        virtual GraphCanvas::Endpoint ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint) const = 0;

        virtual void SetOriginalToNewIdsMap(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& originalIdToNewIds) = 0;
        virtual void GetOriginalToNewIdsMap(AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& originalIdToNewIds) const = 0;
        virtual AZ::EntityId FindNewIdFromOriginal(const AZ::EntityId& originalId) const = 0;
        virtual AZ::EntityId FindOriginalIdFromNew(const AZ::EntityId& newId) const = 0;
    };

    using EditorGraphRequestBus = AZ::EBus<EditorGraphRequests>;

    class EditorGraphNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        virtual void OnGraphCanvasSceneDisplayed() {};
    };

    using EditorGraphNotificationBus = AZ::EBus<EditorGraphNotifications>;

    class EditorNodeNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnGraphCanvasNodeDisplayed(AZ::EntityId /*graphCanvasNodeId*/) {}

        virtual void OnVersionConversionBegin() {};
        virtual void OnVersionConversionEnd() {};
    };

    using EditorNodeNotificationBus = AZ::EBus<EditorNodeNotifications>;

    // Mainly expected to be used from an aggregator.
    class EditorScriptCanvasComponentLogging 
        : public AZ::ComponentBus
    {
    public:
        
        virtual AZ::NamedEntityId FindNamedEntityId() const = 0;
        virtual ScriptCanvas::GraphIdentifier GetGraphIdentifier() const = 0;
    };

    using EditorScriptCanvasComponentLoggingBus = AZ::EBus<EditorScriptCanvasComponentLogging>;

    class EditorLoggingComponentNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OnEditorScriptCanvasComponentActivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;
        virtual void OnEditorScriptCanvasComponentDeactivated(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& graphIdentifier) = 0;
        virtual void OnAssetSwitched(const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& newGraphIdentifier, const ScriptCanvas::GraphIdentifier& oldGraphIdentifier) = 0;
    };

    using EditorLoggingComponentNotificationBus = AZ::EBus<EditorLoggingComponentNotifications>;

    class UpgradeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnUpgradeStart() {}
        virtual void OnUpgradeCancelled() {}

        virtual void OnGraphUpgradeComplete(SourceHandle&, bool skipped = false) { (void)skipped; }
    };

    using UpgradeNotificationsBus = AZ::EBus<UpgradeNotifications>;
}
