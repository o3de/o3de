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

#include <Editor/Assets/ScriptCanvasAssetHolder.h>

#include <ScriptCanvas/Asset/AssetRegistry.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>
#include <Editor/Assets/ScriptCanvasAssetTracker.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerDefinitions.h>

namespace ScriptCanvasEditor
{
    //=========================================================================
    void ScriptCanvasAssetHolder::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptCanvasAssetHolder>()
                ->Version(1)
                ->Field("m_asset", &ScriptCanvasAssetHolder::m_scriptCanvasAsset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptCanvasAssetHolder>("Script Canvas", "Script Canvas Asset Holder")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptCanvasAssetHolder::m_scriptCanvasAsset, "Script Canvas Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptCanvasAssetHolder::OnScriptChanged)
                    ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                    ->Attribute("EditButton", "")
                    ->Attribute("EditDescription", "Open in Script Canvas Editor")
                    ->Attribute("EditCallback", &ScriptCanvasAssetHolder::LaunchScriptCanvasEditor)
                    ;
            }
        }
    }

    ScriptCanvasAssetHolder::~ScriptCanvasAssetHolder()
    {
    }

    void ScriptCanvasAssetHolder::Init(AZ::EntityId ownerId, AZ::ComponentId componentId)
    {
        m_ownerId = AZStd::make_pair(ownerId, componentId);        

        if (!m_scriptCanvasAsset || !m_scriptCanvasAsset.IsReady())
        {
            AssetTrackerNotificationBus::Handler::BusConnect(m_scriptCanvasAsset.GetId());

            Callbacks::OnAssetReadyCallback onAssetReady = [this](ScriptCanvasMemoryAsset& asset)
            {
                AssetHelpers::DumpAssetInfo(asset.GetFileAssetId(), "ScriptCanvasAssetHolder::Init");
            };
            AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::Load, m_scriptCanvasAsset.GetId(), azrtti_typeid<ScriptCanvasAsset>(), onAssetReady);
        }
    }

    void ScriptCanvasAssetHolder::LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&) const
    {
        OpenEditor();
    }

    void ScriptCanvasAssetHolder::OpenEditor() const
    {
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());

        if (m_scriptCanvasAsset.GetId().IsValid())
        {
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, m_scriptCanvasAsset.GetId(), -1);

            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        }
        else if (m_ownerId.first.IsValid())
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

    ScriptCanvas::ScriptCanvasId ScriptCanvasAssetHolder::GetScriptCanvasId() const
    {
        ScriptCanvas::ScriptCanvasId graphId;
        if (m_scriptCanvasAsset.IsReady())
        {
            ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindScriptCanvasId, m_scriptCanvasAsset.Get()->GetScriptCanvasEntity());
        }

        return graphId;
    }

    void ScriptCanvasAssetHolder::SetScriptChangedCB(const ScriptChangedCB& scriptChangedCB)
    {
        m_scriptNotifyCallback = scriptChangedCB;
    }

    void ScriptCanvasAssetHolder::Load(AZ::Data::AssetId fileAssetId)
    {
        if (!m_scriptCanvasAsset.IsReady())
        {
            auto asset = AZ::Data::AssetManager::Instance().FindAsset(fileAssetId, AZ::Data::AssetLoadBehavior::Default);

            if (!asset)
            {
                ScriptCanvasMemoryAsset::pointer memoryAsset;
                AssetTrackerRequestBus::BroadcastResult(memoryAsset, &AssetTrackerRequests::GetAsset, fileAssetId);
                if (!memoryAsset)
                {
                    Callbacks::OnAssetReadyCallback onAssetReady = [this](ScriptCanvasMemoryAsset& asset)
                    {
                        m_memoryScriptCanvasAsset = asset.GetAsset();
                        AssetHelpers::DumpAssetInfo(asset.GetFileAssetId(), "ScriptCanvasAssetHolder::Load onAssetReady");
                    };

                    AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::Load, fileAssetId, azrtti_typeid<ScriptCanvasAsset>(), onAssetReady);
                }
                else
                {
                    AssetHelpers::DumpAssetInfo(fileAssetId, "ScriptCanvasAssetHolder::Load isLoaded");
                    m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().GetAsset(fileAssetId, azrtti_typeid<ScriptCanvasAsset>(), m_scriptCanvasAsset.GetAutoLoadBehavior());
                }
            }

        }
    }

    AZ::u32 ScriptCanvasAssetHolder::OnScriptChanged()
    {
        AssetTrackerNotificationBus::Handler::BusDisconnect();

        if (m_scriptCanvasAsset.GetId().IsValid())
        {
            AssetTrackerNotificationBus::Handler::BusConnect(m_scriptCanvasAsset.GetId());
            Load(m_scriptCanvasAsset.GetId());
        }
        else
        {
            m_scriptCanvasAsset = {};
        }

        if (m_scriptNotifyCallback)
        {
            m_scriptNotifyCallback(m_scriptCanvasAsset.GetId());
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void ScriptCanvasAssetHolder::OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset)
    {
        if (asset->GetFileAssetId() == m_scriptCanvasAsset.GetId())
    {
            AssetTrackerNotificationBus::Handler::BusDisconnect(m_scriptCanvasAsset.GetId());

            m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(asset->GetFileAssetId(), m_scriptCanvasAsset.GetAutoLoadBehavior());
            m_memoryScriptCanvasAsset = asset->GetAsset();
        }
    }

    void ScriptCanvasAssetHolder::SetAsset(AZ::Data::AssetId fileAssetId)
    {
        AssetTrackerNotificationBus::Handler::BusDisconnect();

        m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(fileAssetId, m_scriptCanvasAsset.GetAutoLoadBehavior());
        if (!m_scriptCanvasAsset)
        {
            m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().GetAsset(fileAssetId, azrtti_typeid<ScriptCanvasAsset>(), m_scriptCanvasAsset.GetAutoLoadBehavior());
        }

        if (m_scriptCanvasAsset)
        {
            AssetTrackerNotificationBus::Handler::BusConnect(m_scriptCanvasAsset.GetId());
        }
    }

    const AZ::Data::AssetType& ScriptCanvasAssetHolder::GetAssetType() const
    {
        return m_scriptCanvasAsset.GetType();
    }

    void ScriptCanvasAssetHolder::ClearAsset()
    {
        m_scriptCanvasAsset = {};
    }

    AZ::Data::AssetId ScriptCanvasAssetHolder::GetAssetId() const
    {
        return m_scriptCanvasAsset.GetId();
    }
}
