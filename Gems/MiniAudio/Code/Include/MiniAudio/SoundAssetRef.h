/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/vector.h>
#include <MiniAudio/SoundAsset.h>

namespace MiniAudio
{
    //! A wrapper around SoundAsset that can be used by Script Canvas and Lua
    class SoundAssetRef final
        : private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(SoundAssetRef, "{1edba837-5590-4f2c-a61c-9001eb18505b}");

        static void Reflect(AZ::ReflectContext* context);

        SoundAssetRef() = default;
        ~SoundAssetRef();
        SoundAssetRef(const SoundAssetRef& rhs);
        SoundAssetRef(SoundAssetRef&& rhs);
        SoundAssetRef& operator=(const SoundAssetRef& rhs);
        SoundAssetRef& operator=(SoundAssetRef&& rhs);

        void SetAsset(const AZ::Data::Asset<SoundAsset>& asset);
        AZ::Data::Asset<SoundAsset> GetAsset() const;

    private:
        class SerializationEvents : public AZ::SerializeContext::IEventHandler
        {
            // OnWriteEnd happens after the object pointed at by classPtr
            // has finished being deserialized and is fully loaded.
            void OnWriteEnd(void* classPtr) override
            {
                SoundAssetRef* SoundAssetRef = reinterpret_cast<class SoundAssetRef*>(classPtr);
                // Call SetAsset to connect AssetBus handler as soon as m_asset field is set
                SoundAssetRef->SetAsset(SoundAssetRef->m_asset);
            }
        };

        void OnSpawnAssetChanged();
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::Data::Asset<SoundAsset> m_asset;
    };
}
