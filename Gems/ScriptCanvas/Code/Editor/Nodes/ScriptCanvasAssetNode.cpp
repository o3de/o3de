/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"
#include <Editor/Nodes/ScriptCanvasAssetNode.h>

#include <ScriptCanvas/Core/Graph.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

namespace ScriptCanvasEditor
{
    /**
    * \note when we reflecting we can check if the class is inheriting from IEventHandler
    * and use the this->events.
    */
    class ScriptCanvasAssetNodeEventHandler
        : public AZ::SerializeContext::IEventHandler
    {
        void OnReadBegin(void* classPtr)
        {
            auto* scriptCanvasAssetNode = reinterpret_cast<ScriptCanvasAssetNode*>(classPtr);
            scriptCanvasAssetNode->ComputeDataPatch();
        }
    };

    void ScriptCanvasAssetNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ScriptCanvasAssetNode, ScriptCanvas::Node>()
            ->Version(0)
            ->EventHandler<ScriptCanvasAssetNodeEventHandler>()
            ->Field("m_assetInstance", &ScriptCanvasAssetNode::m_scriptCanvasAssetInstance)
            ;

        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext->Class<ScriptCanvasAssetNode>("ScriptCanvas Asset", "Script Canvas Asset Node which contains a reference to another ScriptCanvas graph")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ;
        }
    }

    ScriptCanvasAssetNode::ScriptCanvasAssetNode(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, bool storeAssetDataInternally)
    {
        SetAssetDataStoredInternally(storeAssetDataInternally);
        SetAsset(scriptCanvasAsset);
    }

    ScriptCanvasAssetNode::~ScriptCanvasAssetNode()
    {
    }

    void ScriptCanvasAssetNode::OnInputSignal(const ScriptCanvas::SlotId&)
    {

    }

    void ScriptCanvasAssetNode::OnInit()
    {
        if (GetAsset().GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(GetAsset().GetId());
            AZ::Entity* scriptCanvasEntity = GetScriptCanvasEntity();
            if (scriptCanvasEntity)
            {
                if (scriptCanvasEntity->GetState() == AZ::Entity::State::Constructed)
                {
                    scriptCanvasEntity->Init();
                }
                if (scriptCanvasEntity->GetState() == AZ::Entity::State::Init)
                {
                    scriptCanvasEntity->Activate();
                }
            }
        }
    }

    bool ScriptCanvasAssetNode::GetAssetDataStoredInternally() const
    {
        return m_scriptCanvasAssetInstance.GetReference().GetAssetDataStoredInternally();
    }

    void ScriptCanvasAssetNode::SetAssetDataStoredInternally(bool storeInObjectStream)
    {
        m_scriptCanvasAssetInstance.GetReference().SetAssetDataStoredInternally(storeInObjectStream);
    }

    ScriptCanvas::ScriptCanvasData& ScriptCanvasAssetNode::GetScriptCanvasData()
    {
        return m_scriptCanvasAssetInstance.GetScriptCanvasData();
    }

    const ScriptCanvas::ScriptCanvasData& ScriptCanvasAssetNode::GetScriptCanvasData() const
    {
        return m_scriptCanvasAssetInstance.GetScriptCanvasData();
    }

    AZ::Entity* ScriptCanvasAssetNode::GetScriptCanvasEntity() const
    {
        return GetScriptCanvasData().GetScriptCanvasEntity();
    }

    AZ::Data::Asset<ScriptCanvasAsset>& ScriptCanvasAssetNode::GetAsset()
    {
        return m_scriptCanvasAssetInstance.GetReference().GetAsset();
    }

    const AZ::Data::Asset<ScriptCanvasAsset>& ScriptCanvasAssetNode::GetAsset() const
    {
        return m_scriptCanvasAssetInstance.GetReference().GetAsset();
    }

    void ScriptCanvasAssetNode::SetAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset)
    {
        m_scriptCanvasAssetInstance.GetReference().SetAsset(scriptCanvasAsset);
        if (scriptCanvasAsset.GetId().IsValid())
        {
            auto onAssetReady = [](ScriptCanvasMemoryAsset&) {};
            AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::Load, scriptCanvasAsset.GetId(), azrtti_typeid<ScriptCanvasAsset>(), onAssetReady);

            AZ::Data::AssetBus::Handler::BusConnect(scriptCanvasAsset.GetId());
            ApplyDataPatch();
        }
    }

    void ScriptCanvasAssetNode::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);
        if (!GetAsset().IsReady())
        {
            AZ_Error("Script Canvas", false, "Reloaded graph with id %s is not valid", asset.GetId().ToString<AZStd::string>().data());
            return;
        }

        // Update data patches against the old version of the asset.
        ApplyDataPatch();
    }

    void ScriptCanvasAssetNode::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect(assetId);
    }

    void ScriptCanvasAssetNode::ComputeDataPatch()
    {
        m_scriptCanvasAssetInstance.ComputeDataPatch();
    }

    void ScriptCanvasAssetNode::ApplyDataPatch()
    {
        m_scriptCanvasAssetInstance.ApplyDataPatch();
    }

    bool ScriptCanvasAssetNode::Visit(const VisitCB& preVisitCB, const VisitCB & postVisitCB)
    {
        AZStd::unordered_set<AZ::Data::AssetId> visitedGraphs;
        return Visit(preVisitCB, postVisitCB, visitedGraphs);
    }

    bool ScriptCanvasAssetNode::Visit(const VisitCB& preVisitCB, const VisitCB & postVisitCB, AZStd::unordered_set<AZ::Data::AssetId>& visitedGraphs)
    {
        const AZ::Data::AssetId& visitedAssetId = GetAsset().GetId();
        if (visitedGraphs.count(visitedAssetId) > 0)
        {
            AZ_Error("Script Canvas", false, "The Script Canvas Asset %s has already been visited, processing will stop", visitedAssetId.ToString<AZStd::string>().data());
            return false;
        }

        if (!preVisitCB || preVisitCB(*this))
        {
            auto graph = GetScriptCanvasEntity() ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(GetScriptCanvasEntity()) : nullptr;
            if (graph)
            {
                for (AZ::Entity* nodeEntity : graph->GetNodeEntities())
                {
                    if (auto childAssetNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasAssetNode>(nodeEntity))
                    {
                        // Visit ScriptCanvasAssetNodes that are in the asset referenced by @assetNode
                        if (!childAssetNode->Visit(preVisitCB, postVisitCB, visitedGraphs))
                        {
                            break;
                        }

                    }
                }
            }
        }

        // Visit siblings ScriptCanvasAssetNodes
        bool visitSiblings = !postVisitCB || postVisitCB(*this);
        visitedGraphs.insert(visitedAssetId);
        return visitSiblings;
    }
}
