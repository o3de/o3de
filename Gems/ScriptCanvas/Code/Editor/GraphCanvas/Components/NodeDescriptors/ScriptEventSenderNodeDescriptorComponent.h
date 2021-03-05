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
