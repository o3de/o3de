/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include "ScriptEvents/ScriptEventsAssetRef.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptEvents
{
    void ScriptEventsAssetRef::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptEventsAssetRef>()->Version(0)->Field("Asset", &ScriptEventsAssetRef::m_asset);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptEventsAssetRef>("Script Event Asset", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEventsAssetRef::m_asset, "Script Event Asset", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptEventsAssetRef::OnAssetChanged)
                    // TODO #lsempe: hook up to open Asset Editor when ready
                    //->Attribute("EditButton", "")
                    //->Attribute("EditDescription", "Open in Script Canvas Editor")
                    //->Attribute("EditCallback", &ScriptEventsAssetRef::LaunchScriptCanvasEditor)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ScriptEventsAssetRef>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ConstructibleFromNil, false)
                ->Method("Get", &ScriptEventsAssetRef::GetDefinition);
        }
    }

    void ScriptEventsAssetRef::SetAsset(const AZ::Data::Asset<ScriptEventsAsset>& asset)
    {
        m_asset = asset;

        if (m_asset.IsReady())
        {
            if (ScriptEventsAsset* scriptEventAsset = m_asset.GetAs<ScriptEventsAsset>())
            {
                scriptEventAsset->m_definition.RegisterInternal();
            }
        }
        else
        {
            if (AZ::Data::AssetBus::Handler::BusIsConnectedId(m_asset.GetId()))
            {
                AZ::Data::AssetBus::Handler::BusDisconnect(m_asset.GetId());
            }

            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
        }
    }

    void ScriptEventsAssetRef::Load(bool loadBlocking /*= false*/)
    {
        if (!m_asset.IsReady())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_asset.GetId());
            if (assetInfo.m_assetId.IsValid())
            {
                auto& assetManager = AZ::Data::AssetManager::Instance();
                m_asset = assetManager.GetAsset(m_asset.GetId(), azrtti_typeid<ScriptEventsAsset>(), m_asset.GetAutoLoadBehavior());

                if(loadBlocking)
                {
                    m_asset.BlockUntilLoadComplete();
                }
            }
        }
    }

    AZ::u32 ScriptEventsAssetRef::OnAssetChanged()
    {
        SetAsset(m_asset);
        Load(false);

        if (m_assetNotifyCallback)
        {
            m_assetNotifyCallback(m_asset, m_userData);
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void ScriptEventsAssetRef::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (ScriptEventsAsset* scriptEventAsset = m_asset.GetAs<ScriptEventsAsset>())
        {
            scriptEventAsset->m_definition.RegisterInternal();
        }
    }

    void ScriptEventsAssetRef::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SetAsset(asset);

        if (m_assetNotifyCallback)
        {
            m_assetNotifyCallback(m_asset, m_userData);
        }
    }

    void ScriptEventsAssetRef::OnAssetUnloaded(
        [[maybe_unused]] const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType)
    {
        if (ScriptEventsAsset* ebusAsset = m_asset.GetAs<ScriptEventsAsset>())
        {
            bool isRegistered = false;
            // ScriptEventsLegacy::RegistrationRequestBus::BroadcastResult(isRegistered,
            // &ScriptEventsLegacy::RegistrationRequestBus::Events::IsBusRegistered, ebusAsset->m_scriptEventsDefinition.m_name);
            if (isRegistered)
            {
                // ScriptEventsLegacy::RegistrationRequestBus::Broadcast(&ScriptEventsLegacy::RegistrationRequestBus::Events::Unregister,
                // ebusAsset->m_scriptEventsDefinition.m_name);
            }
        }
    }
} // namespace ScriptEvents
