/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptEvents
{

    /**
    * Provides script bindings to expose Script Event assets as script property.
    */
    class ScriptEventsAssetRef
        : private AZ::Data::AssetBus::Handler
    {

    public:

        AZ_RTTI(ScriptEventsAssetRef, "{9BF12D72-9FE5-4F0E-A115-B92D99FB1CD7}");
        AZ_CLASS_ALLOCATOR(ScriptEventsAssetRef, AZ::SystemAllocator, 0);

        using AssetChangedCB = AZStd::function<void(const AZ::Data::Asset<ScriptEventsAsset>&, void* userData)>;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptEventsAssetRef>()
                    ->Version(0)
                    ->Field("Asset", &ScriptEventsAssetRef::m_asset)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ScriptEventsAssetRef>("Script Event Asset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEventsAssetRef::m_asset, "Script Event Asset", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptEventsAssetRef::OnAssetChanged)
                        //TODO #lsempe: hook up to open Asset Editor when ready
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
                    ->Method("Get", &ScriptEventsAssetRef::GetDefinition)
                    ;
            }
        }

        ScriptEventsAssetRef() = default;

        ScriptEventsAssetRef(AZ::Data::Asset<ScriptEventsAsset> asset, const AssetChangedCB& assetChangedCB, void* userData)
            : m_asset(asset)
            , m_assetNotifyCallback(assetChangedCB)
            , m_userData(userData)
        {
            SetAsset(asset);
        }

        ~ScriptEventsAssetRef()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        const ScriptEvents::ScriptEvent* GetDefinition() const
        {
            if (ScriptEventsAsset* ebusAsset = m_asset.GetAs<ScriptEventsAsset>())
            {
                ebusAsset->m_definition.RegisterInternal();

                return &ebusAsset->m_definition;
            }

            return nullptr;
        }

        void SetAsset(const AZ::Data::Asset<ScriptEventsAsset>& asset)
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

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> GetAsset() const
        {
            return m_asset;
        }

        void Load(bool loadBlocking /*= false*/)
        {
            if (!m_asset.IsReady())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_asset.GetId());
                if (assetInfo.m_assetId.IsValid())
                {
                    const AZ::Data::AssetType assetTypeId = azrtti_typeid<ScriptEventsAsset>();
                    auto& assetManager = AZ::Data::AssetManager::Instance();
                    m_asset = assetManager.GetAsset(m_asset.GetId(), azrtti_typeid<ScriptEventsAsset>(), m_asset.GetAutoLoadBehavior());

                    if(loadBlocking)
                    {
                        m_asset.BlockUntilLoadComplete();
                    }
                }
            }
        }

        AZ::u32 OnAssetChanged()
        {
            SetAsset(m_asset);
            Load(false);

            if (m_assetNotifyCallback)
            {
                m_assetNotifyCallback(m_asset, m_userData);
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        //=====================================================================
        // AZ::Data::AssetBus

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            if (ScriptEventsAsset* scriptEventAsset = m_asset.GetAs<ScriptEventsAsset>())
            {
                scriptEventAsset->m_definition.RegisterInternal();
            }
        }

        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            SetAsset(asset);

            if (m_assetNotifyCallback)
            {
                m_assetNotifyCallback(m_asset, m_userData);
            }
        }

        void OnAssetUnloaded([[maybe_unused]] const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType) override
        {
            if (ScriptEventsAsset* ebusAsset = m_asset.GetAs<ScriptEventsAsset>())
            {
                bool isRegistered = false;
                //ScriptEventsLegacy::RegistrationRequestBus::BroadcastResult(isRegistered, &ScriptEventsLegacy::RegistrationRequestBus::Events::IsBusRegistered, ebusAsset->m_scriptEventsDefinition.m_name);
                if (isRegistered)
                {
                    //ScriptEventsLegacy::RegistrationRequestBus::Broadcast(&ScriptEventsLegacy::RegistrationRequestBus::Events::Unregister, ebusAsset->m_scriptEventsDefinition.m_name);
                }
            }
        }

        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] bool isSuccessful) override
        {
            SetAsset(m_asset);
        }
        //=====================================================================

        private:

            AssetChangedCB m_assetNotifyCallback;
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;
            void* m_userData;
    };
}
