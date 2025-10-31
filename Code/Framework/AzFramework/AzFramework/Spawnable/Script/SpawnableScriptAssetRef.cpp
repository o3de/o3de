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
                ->Version(0)
                ->EventHandler<SerializationEvents>()
                ->Field("asset", &SpawnableScriptAssetRef::m_asset)
            ;

            serializeContext->RegisterGenericType<AZStd::vector<SpawnableScriptAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, SpawnableScriptAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<double, SpawnableScriptAssetRef>>(); // required to support Map<Number, SpawnableScriptAssetRef> in Script Canvas

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SpawnableScriptAssetRef>("SpawnableScriptAssetRef", "A wrapper around spawnable asset to be used as a variable in Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
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
            behaviorContext->Class<SpawnableScriptAssetRef>("SpawnableScriptAssetRef")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, true)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Constructor()
                ->Method("GetAsset", &SpawnableScriptAssetRef::GetAsset)
                ->Method("SetAsset", &SpawnableScriptAssetRef::SetAsset)
                ->Method("GetAssetId", &SpawnableScriptAssetRef::GetAssetId)
                ->Method("SetAssetId", &SpawnableScriptAssetRef::SetAssetId)
                ->Method("IsValid", &SpawnableScriptAssetRef::IsValid)
                ;
        }
    }

    SpawnableScriptAssetRef::~SpawnableScriptAssetRef()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    SpawnableScriptAssetRef::SpawnableScriptAssetRef(const SpawnableScriptAssetRef& rhs)
    {
        SetAsset(rhs.m_asset);
    }

    SpawnableScriptAssetRef::SpawnableScriptAssetRef(SpawnableScriptAssetRef&& rhs)
    {
        SetAsset(AZStd::move(rhs.m_asset));
    }

    SpawnableScriptAssetRef& SpawnableScriptAssetRef::operator=(const SpawnableScriptAssetRef& rhs)
    {
        if (this != &rhs)
        {
            SetAsset(rhs.m_asset);
        }
        return *this;
    }

    SpawnableScriptAssetRef& SpawnableScriptAssetRef::operator=(SpawnableScriptAssetRef&& rhs)
    {
        if (this != &rhs)
        {
            SetAsset(AZStd::move(rhs.m_asset));
        }
        return *this;
    }

    void SpawnableScriptAssetRef::SetAsset(const AZ::Data::Asset<Spawnable>& asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_asset = asset;
        if (m_asset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
        }
    }

    AZ::Data::Asset<Spawnable> SpawnableScriptAssetRef::GetAsset() const
    {
        return m_asset;
    }

    void SpawnableScriptAssetRef::OnSpawnAssetChanged()
    {
        SetAsset(m_asset);
    }

    void SpawnableScriptAssetRef::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;
    }

    void SpawnableScriptAssetRef::SetAssetId(const AZ::Data::AssetId& assetId)
    {
        if (assetId == m_asset.GetId())
        {
            return;
        }

        AZ::Data::Asset<Spawnable> newAsset =
            AZ::Data::AssetManager::Instance().GetAsset<Spawnable>(assetId, AZ::Data::AssetLoadBehavior::NoLoad);
        SetAsset(newAsset);
    }

    AZ::Data::AssetId SpawnableScriptAssetRef::GetAssetId() const
    {
        return m_asset.GetId();
    }

    bool SpawnableScriptAssetRef::IsValid() const
    {
        return m_asset.GetId().IsValid();
    }
}
