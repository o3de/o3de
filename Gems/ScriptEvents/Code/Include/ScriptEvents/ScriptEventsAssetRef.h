/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>

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
        AZ_CLASS_ALLOCATOR(ScriptEventsAssetRef, AZ::SystemAllocator);

        using AssetChangedCB = AZStd::function<void(const AZ::Data::Asset<ScriptEventsAsset>&, void* userData)>;

        static void Reflect(AZ::ReflectContext* context);

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

        void SetAsset(const AZ::Data::Asset<ScriptEventsAsset>& asset);

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> GetAsset() const
        {
            return m_asset;
        }

        void Load(bool loadBlocking /*= false*/);
        AZ::u32 OnAssetChanged();

        //=====================================================================
        // AZ::Data::AssetBus

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;

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
