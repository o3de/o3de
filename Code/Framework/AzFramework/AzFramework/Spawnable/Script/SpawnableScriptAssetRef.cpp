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
#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework::Scripts
{
    void SpawnableScriptAssetRef::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<SpawnableScriptAssetRef>()
                ->Field("asset", &SpawnableScriptAssetRef::m_asset);

            serializeContext->RegisterGenericType<AZStd::vector<SpawnableScriptAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, SpawnableScriptAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<double, SpawnableScriptAssetRef>>(); // required to support Map<Number, SpawnableScriptAssetRef> in Script Canvas

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SpawnableScriptAssetRef>("SpawnableScriptAssetRef", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    // m_asset
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnableScriptAssetRef::m_asset, "asset", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Spawnable Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SpawnableScriptAssetRef::OnSpawnAssetChanged);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->Class<SpawnableScriptAssetRef>("SpawnableScriptAssetRef")
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Property("asset", BehaviorValueProperty(&SpawnableScriptAssetRef::m_asset));
        }
    }

    SpawnableScriptAssetRef::~SpawnableScriptAssetRef()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    SpawnableScriptAssetRef::SpawnableScriptAssetRef(const SpawnableScriptAssetRef& rhs)
        : m_asset(rhs.m_asset)
    {
    }

    SpawnableScriptAssetRef::SpawnableScriptAssetRef(SpawnableScriptAssetRef&& rhs)
        : m_asset(AZStd::move(rhs.m_asset))
    {
    }

    SpawnableScriptAssetRef& SpawnableScriptAssetRef::operator=(const SpawnableScriptAssetRef& rhs)
    {
        if (this != &rhs)
        {
            m_asset = rhs.m_asset;
        }
        return *this;
    }

    SpawnableScriptAssetRef& SpawnableScriptAssetRef::operator=(SpawnableScriptAssetRef&& rhs)
    {
        if (this != &rhs)
        {
            m_asset = AZStd::move(rhs.m_asset);
        }
        return *this;
    }

    void SpawnableScriptAssetRef::SetAsset(const AZ::Data::Asset<Spawnable>& asset)
    {
        m_asset = asset;
    }

    AZ::Data::Asset<Spawnable> SpawnableScriptAssetRef::GetAsset() const
    {
        return m_asset;
    }

    void SpawnableScriptAssetRef::OnSpawnAssetChanged()
    {
        // Disconnect from the bus beforehand in case the new asset is not valid
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (m_asset.GetId().IsValid())
        {
            AZStd::string spawnableAssetFile;
            StringFunc::Path::GetFileName(m_asset.GetHint().c_str(), spawnableAssetFile);
            StringFunc::Path::ReplaceExtension(spawnableAssetFile, Spawnable::DotFileExtension);
            AZ::u32 spawnableAssetSubId = SpawnableAssetHandler::BuildSubId(spawnableAssetFile);

            if (m_asset.GetId().m_subId != spawnableAssetSubId)
            {
                AZ::Data::AssetId spawnableAssetId = m_asset.GetId();
                spawnableAssetId.m_subId = spawnableAssetSubId;

                m_asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<Spawnable>(
                    spawnableAssetId, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);
            }

            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
        }
    }

    void SpawnableScriptAssetRef::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;
    }
}
