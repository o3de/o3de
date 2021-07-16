/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptCanvasEditor
{
    class ScriptEventSenderNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public AZ::Data::AssetBus::Handler
        , public EditorNodeNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptEventSenderNodeDescriptorComponent, "{7EB63D67-4F32-40E5-8B15-4C3E28D886F9}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        ScriptEventSenderNodeDescriptorComponent();
        ScriptEventSenderNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);
        ~ScriptEventSenderNodeDescriptorComponent() = default;

        // Component
        void Activate() override;
        void Deactivate() override;
        ////
        
        // AZ::Data::AssetBus::Handler
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // VersionControlledNodeRequestBus::Handler
        void OnVersionConversionBegin() override;
        void OnVersionConversionEnd() override;
        ////

    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:

        void UpdateTitles();
        void SignalNeedsVersionConversion();

        AZ::EntityId m_scriptCanvasId;
    
        AZ::Data::AssetId m_assetId;
        ScriptCanvas::EBusEventId m_eventId;
    };
}
