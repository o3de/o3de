/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    class FunctionNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public AZ::Data::AssetBus::Handler
        , public EditorNodeNotificationBus::Handler
        , public GraphCanvas::VisualNotificationBus::Handler
    {
    public:    
        AZ_COMPONENT(FunctionNodeDescriptorComponent, "{B9DA0350-AF62-475E-8DD7-30E8F4F313BB}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        FunctionNodeDescriptorComponent();
        FunctionNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const AZStd::string& name);
        ~FunctionNodeDescriptorComponent() = default;

        // Component
        void Activate() override;
        void Deactivate() override;
        ////
        
        // AZ::Data::AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // VersionControlledNodeRequestBus::Handler
        void OnVersionConversionEnd() override;
        ////

        // GraphCanvas::VisualNotificationBus
        bool OnMouseDoubleClick(const QGraphicsSceneMouseEvent*) override;
        //


    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:

        void TriggerUpdate();

        void UpdateTitles();

        AZ::EntityId m_scriptCanvasId;

        AZ::Data::AssetId m_assetId;

        AZStd::string m_name;
    };
}
