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
#include <GraphCanvas/Components/VisualBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Asset/Functions/ScriptCanvasFunctionAsset.h>
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
        ////

        // VersionControlledNodeRequestBus::Handler
        void OnVersionConversionEnd() override;
        ////

        // GraphCanvas::NodeNotificationBus
        bool OnMouseDoubleClick(const QGraphicsSceneMouseEvent*) override;
        //


    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:

        void UpdateTitles();

        AZ::EntityId m_scriptCanvasId;

        AZ::Data::AssetId m_assetId;

        AZStd::string m_name;
    };
}
