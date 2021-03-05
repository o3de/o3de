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
#include "precompiled.h"

#include "FunctionNodeDescriptorComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Translation/TranslationHelper.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>
#include <ScriptCanvas/Libraries/Core/FunctionNode.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////////////
    // FunctionNodeDescriptorComponent
    /////////////////////////////////////////////

    void FunctionNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<FunctionNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("AssetId", &FunctionNodeDescriptorComponent::m_assetId)
                ;
        }
    }

    FunctionNodeDescriptorComponent::FunctionNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::FunctionNode)
    {
    }

    FunctionNodeDescriptorComponent::FunctionNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const AZStd::string& name)
        : NodeDescriptorComponent(NodeDescriptorType::FunctionNode)
        , m_assetId(assetId)
        , m_name(name)
    {
        
    }

    void FunctionNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();
        GraphCanvas::VisualNotificationBus::Handler::BusConnect(GetEntityId());
        
        AZ::Data::AssetBus::Handler::BusConnect(m_assetId);
    }

    void FunctionNodeDescriptorComponent::Deactivate()
    {
        GraphCanvas::VisualNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void FunctionNodeDescriptorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        EditorGraphRequestBus::Event(scriptCanvasId, &EditorGraphRequests::QueueVersionUpdate, GetEntityId());
    }

    void FunctionNodeDescriptorComponent::OnVersionConversionEnd()
    {
        AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::RuntimeFunctionAsset>(m_assetId, AZ::Data::AssetLoadBehavior::Default);

        asset.BlockUntilLoadComplete();

        if (asset.IsReady())
        {
            //UpdateTitles();

            // For reference:
            //if (!asset.Get()->m_definition.HasMethod(m_eventId))
            //{
            //    AZStd::unordered_set< AZ::EntityId > deleteNodes = { GetEntityId() };
            //    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
            //}
        }
    }

    bool FunctionNodeDescriptorComponent::OnMouseDoubleClick(const QGraphicsSceneMouseEvent*)
    {
        AZ::Data::AssetInfo assetInfo = AssetHelpers::GetSourceInfoByProductId(m_assetId, azrtti_typeid< ScriptCanvas::RuntimeFunctionAsset>());

        if (!assetInfo.m_assetId.IsValid())
        {
            return false;
        }

        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
        GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, assetInfo.m_assetId, -1);
        return openOutcome.IsSuccess();
    }

    void FunctionNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        m_scriptCanvasId = scriptCanvasNodeId;
        EditorNodeNotificationBus::Handler::BusConnect(m_scriptCanvasId);
    }

    void FunctionNodeDescriptorComponent::UpdateTitles()
    {
        GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, m_name);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, m_name);
    }
}
