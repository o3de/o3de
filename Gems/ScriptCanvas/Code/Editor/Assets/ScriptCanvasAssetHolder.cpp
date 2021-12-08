/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
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

            Callbacks::OnAssetReadyCallback onAssetReady = [](ScriptCanvasMemoryAsset& asset)
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
        m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(fileAssetId, AZ::Data::AssetLoadBehavior::Default);

        if (!m_scriptCanvasAsset || !m_scriptCanvasAsset.IsReady())
        {
            m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().GetAsset(fileAssetId, azrtti_typeid<ScriptCanvasAsset>(), AZ::Data::AssetLoadBehavior::Default);
            m_triggeredLoad = true;

            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(fileAssetId);
        }
        else if (m_memoryScriptCanvasAsset.Get() == nullptr)
        {
            m_triggeredLoad = false;
            LoadMemoryAsset(fileAssetId);
        }
    }

    void ScriptCanvasAssetHolder::LoadMemoryAsset(AZ::Data::AssetId fileAssetId)
    {
        Callbacks::OnAssetReadyCallback onAssetReady = [this](ScriptCanvasMemoryAsset& asset)
        {
            m_memoryScriptCanvasAsset = asset.GetAsset();
            AssetHelpers::DumpAssetInfo(asset.GetFileAssetId(), "ScriptCanvasAssetHolder::Load onAssetReady");
        };

        AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::Load, fileAssetId, azrtti_typeid<ScriptCanvasAsset>(), onAssetReady);
    }

    void ScriptCanvasAssetHolder::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        LoadMemoryAsset(asset.GetId());
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
            m_memoryScriptCanvasAsset = {};
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

            m_scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(asset->GetFileAssetId(), AZ::Data::AssetLoadBehavior::Default);
            m_memoryScriptCanvasAsset = asset->GetAsset();

            if (m_triggeredLoad && m_scriptNotifyCallback)
            {
                m_triggeredLoad = false;
                m_scriptNotifyCallback(m_scriptCanvasAsset.GetId());
            }
        }
    }

    void ScriptCanvasAssetHolder::SetAsset(AZ::Data::AssetId fileAssetId)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AssetTrackerNotificationBus::Handler::BusDisconnect();

        Load(fileAssetId);

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
        m_memoryScriptCanvasAsset = {};
    }

    AZ::Data::AssetId ScriptCanvasAssetHolder::GetAssetId() const
    {
        return m_scriptCanvasAsset.GetId();
    }
}
