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
    class SpawnableScriptAssetRef
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

        void SetAsset(const AZ::Data::Asset<Spawnable>& asset);
        AZ::Data::Asset<Spawnable> GetAsset() const;

    protected:
        // Prefabs can generate multiple .spawnable products. This class will by default use the first .spawnable generated
        // from each prefab. If other subsystems (such as networking) generate more types of .spawnables and want to make them
        // individually selectable, the asset filter controls are exposed here so that subclasses can change how the assets are
        // displayed and filtered.
        virtual bool ShowProductAssetFileName() const;
        virtual bool HideProductAssetFiles() const;
        virtual const char* GetAssetPickerTitle() const;
        virtual AZ::Outcome<void, AZStd::string> ValidatePotentialSpawnableAsset(void* newValue, const AZ::Uuid& valueType) const;

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
