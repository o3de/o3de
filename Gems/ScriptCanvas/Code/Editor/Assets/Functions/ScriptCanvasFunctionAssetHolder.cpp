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

#include <LyViewPaneNames.h>

#include <Editor/Assets/Functions/ScriptCanvasFunctionAssetHolder.h>

#include <ScriptCanvas/Asset/AssetRegistry.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    void ScriptCanvasFunctionAssetHolder::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptCanvasFunctionAssetHolder>()
                ->Version(1)
                ->Field("m_asset", &ScriptCanvasFunctionAssetHolder::m_scriptCanvasAssetId)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptCanvasFunctionAssetHolder>("Script Canvas", "Script Canvas Function Asset Holder")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasFunctionAssetHolder::m_scriptCanvasAssetId, "Script Canvas Function Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptCanvasFunctionAssetHolder::OnScriptChanged)
                    ->Attribute("EditButton", "Editor/Icons/PropertyEditor/open_in.png")
                    ->Attribute("EditDescription", "Open in Script Canvas Editor")
                    ->Attribute("EditCallback", &ScriptCanvasFunctionAssetHolder::LaunchScriptCanvasEditor)
                    ;
            }
        }
    }

    ScriptCanvasFunctionAssetHolder::ScriptCanvasFunctionAssetHolder(AZ::Data::AssetId assetId, const ScriptChangedCB& scriptChangedCB)
        : m_scriptCanvasAssetId(assetId)
        , m_scriptNotifyCallback(scriptChangedCB)
    {
    }

    ScriptCanvasFunctionAssetHolder::~ScriptCanvasFunctionAssetHolder()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ScriptCanvasFunctionAssetHolder::Init(AZ::EntityId ownerId)
    {
        m_ownerId = ownerId;

        if (m_scriptCanvasAssetId.IsValid())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(m_scriptCanvasAssetId);
        }
    }

    void ScriptCanvasFunctionAssetHolder::LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&) const
    {
        OpenEditor();
    }

    void ScriptCanvasFunctionAssetHolder::OpenEditor() const
    {
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());

        if (m_scriptCanvasAssetId.IsValid())
        {
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, m_scriptCanvasAssetId, -1);

            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        }
        else if (m_ownerId.IsValid())
        {
            AzToolsFramework::EntityIdList selectedEntityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            // Going to bypass the multiple selected entities flow for right now.
            if (selectedEntityIds.size() == 1)
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::CreateScriptCanvasAssetFor, m_ownerId);
            }
        }        
    }

    AZ::EntityId ScriptCanvasFunctionAssetHolder::GetGraphId() const
    {
        AZ::EntityId graphId;
        if (m_scriptCanvasAsset.IsReady())
        {
            ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, m_scriptCanvasAsset.Get()->GetScriptCanvasEntity());
        }
        return graphId;
    }

    void ScriptCanvasFunctionAssetHolder::SetScriptChangedCB(const ScriptChangedCB& scriptChangedCB)
    {
        m_scriptNotifyCallback = scriptChangedCB;
    }

    void ScriptCanvasFunctionAssetHolder::Load(bool loadBlocking)
    {
        if (!m_scriptCanvasAsset.IsReady())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_scriptCanvasAsset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                const AZ::Data::AssetType assetTypeId = azrtti_typeid<ScriptCanvasAsset>();
                auto& assetManager = AZ::Data::AssetManager::Instance();
                m_scriptCanvasAsset = assetManager.GetAsset(m_scriptCanvasAsset.GetId(), azrtti_typeid<ScriptCanvasAsset>(), true, nullptr, loadBlocking);
            }
        }
    }

    AZ::u32 ScriptCanvasFunctionAssetHolder::OnScriptChanged()
    {
        SetAsset(m_scriptCanvasAsset.GetId());
        Load(false);

        if (m_scriptNotifyCallback)
        {
            m_scriptNotifyCallback(m_scriptCanvasAsset);
        }
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void ScriptCanvasFunctionAssetHolder::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset.GetId());
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReady, m_scriptCanvasAsset);
    }

    void ScriptCanvasFunctionAssetHolder::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset.GetId());
        EditorScriptCanvasAssetNotificationBus::Event(m_scriptCanvasAsset.GetId(), &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetReloaded, m_scriptCanvasAsset);
    }

    void ScriptCanvasFunctionAssetHolder::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        EditorScriptCanvasAssetNotificationBus::Event(assetId, &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetUnloaded, assetId);
    }

    void ScriptCanvasFunctionAssetHolder::OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool isSuccessful)
    {
        SetAsset(asset.GetId());
        auto currentBusId = AZ::Data::AssetBus::GetCurrentBusId();
        if (currentBusId)
        {
            EditorScriptCanvasAssetNotificationBus::Event(*currentBusId, &EditorScriptCanvasAssetNotifications::OnScriptCanvasAssetSaved, asset.GetId());
        }
    }

    void ScriptCanvasFunctionAssetHolder::SetAsset(AZ::Data::AssetId assetId)
    {
        m_scriptCanvasAssetId = assetId;

        if (!AZ::Data::AssetBus::Handler::BusIsConnectedId(m_scriptCanvasAssetId))
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            if (m_scriptCanvasAssetId.IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_scriptCanvasAssetId);
            }
        }
    }

    //AZ::Data::Asset<ScriptCanvasAsset> ScriptCanvasFunctionAssetHolder::GetAsset() const
    //{
    //    return m_scriptCanvasAsset;
    //}

    AZ::Data::AssetId ScriptCanvasFunctionAssetHolder::GetAssetId() const
    {
        return m_scriptCanvasAssetId;
    }
}
