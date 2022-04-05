/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzFramework/Spawnable/SpawnableAssetRef.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    void SpawnableAssetRef::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<SpawnableAssetRef>()
                ->Field("Asset", &SpawnableAssetRef::m_asset);

            serializeContext->RegisterGenericType<AZStd::vector<SpawnableAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, SpawnableAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<double, SpawnableAssetRef>>(); // required to support Map<Number, SpawnableAssetRef> in Script Canvas

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SpawnableAssetRef>("SpawnableAssetRef", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    // m_asset
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnableAssetRef::m_asset, "asset", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Spawnable Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnableAssetRef::OnSpawnAssetChanged);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->Class<SpawnableAssetRef>("SpawnableAssetRef")
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Property("asset", BehaviorValueProperty(&SpawnableAssetRef::m_asset));
        }
    }

    SpawnableAssetRef::SpawnableAssetRef()
    {
        OnSpawnAssetChanged();
    }

    SpawnableAssetRef::~SpawnableAssetRef()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    SpawnableAssetRef::SpawnableAssetRef(const SpawnableAssetRef& rhs)
        : m_asset(rhs.m_asset)
    {
        OnSpawnAssetChanged();
    }

    SpawnableAssetRef& SpawnableAssetRef::operator=(const SpawnableAssetRef& rhs)
    {
        m_asset = rhs.m_asset;
        OnSpawnAssetChanged();
        return *this;
    }

    void SpawnableAssetRef::OnSpawnAssetChanged()
    {
        // Disconnect from the bus beforehand in case the new asset is not valid
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (m_asset.GetId().IsValid())
        {
            AZStd::string rootSpawnableFile;
            StringFunc::Path::GetFileName(m_asset.GetHint().c_str(), rootSpawnableFile);
            StringFunc::Path::ReplaceExtension(rootSpawnableFile, Spawnable::DotFileExtension);
            AZ::u32 rootSubId = SpawnableAssetHandler::BuildSubId(rootSpawnableFile);

            if (m_asset.GetId().m_subId != rootSubId)
            {
                AZ::Data::AssetId rootAssetId = m_asset.GetId();
                rootAssetId.m_subId = rootSubId;

                m_asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<Spawnable>(
                    rootAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }

            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
        }
    }

    void SpawnableAssetRef::OnAssetReady([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;
        OnSpawnAssetChanged();
    }

    void SpawnableAssetRef::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;
        OnSpawnAssetChanged();
    }
}
