/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework::Scripts
{
    //! A wrapper around Spawnable asset that can be used by Script Canvas and Lua
    class SpawnableScriptAssetRef final
        : private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(AzFramework::Scripts::SpawnableScriptAssetRef, "{A96A5037-AD0D-43B6-9948-ED63438C4A52}");

        static void Reflect(AZ::ReflectContext* context);

        SpawnableScriptAssetRef() = default;
        ~SpawnableScriptAssetRef();
        SpawnableScriptAssetRef(const SpawnableScriptAssetRef& rhs);
        SpawnableScriptAssetRef(SpawnableScriptAssetRef&& rhs);
        SpawnableScriptAssetRef& operator=(const SpawnableScriptAssetRef& rhs);
        SpawnableScriptAssetRef& operator=(SpawnableScriptAssetRef&& rhs);

        bool operator == (const SpawnableScriptAssetRef& rhs) const
        {
            return m_asset == rhs.GetAsset();
        }

        void SetAsset(const AZ::Data::Asset<Spawnable>& asset);
        AZ::Data::Asset<Spawnable> GetAsset() const;

    private:
        class SerializationEvents : public AZ::SerializeContext::IEventHandler
        {
            void OnReadEnd(void* classPtr) override
            {
                SpawnableScriptAssetRef* spawnableScriptAssetRef = reinterpret_cast<SpawnableScriptAssetRef*>(classPtr);
                // Call SetAsset to connect AssetBus handler as soon as m_asset field is set
                spawnableScriptAssetRef->SetAsset(spawnableScriptAssetRef->m_asset);
            }
        };

        void OnSpawnAssetChanged();
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::Data::Asset<Spawnable> m_asset;
    };
}

namespace AZStd
{
    template<>
    struct hash<AzFramework::Scripts::SpawnableScriptAssetRef>
    {
        using argument_type = AzFramework::Scripts::SpawnableScriptAssetRef;
        using result_type = size_t;

        result_type operator() (const argument_type& ticket) const
        {
            size_t h = 0;
            hash_combine(h, ticket.GetAsset().GetId());
            return h;
        }
    };
}
