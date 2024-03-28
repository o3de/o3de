/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MiniAudio/SoundAssetRef.h>

namespace MiniAudio
{
    void SoundAssetRef::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SoundAssetRef>()->Version(0)->EventHandler<SerializationEvents>()->Field(
                "asset", &SoundAssetRef::m_asset);

            serializeContext->RegisterGenericType<AZStd::vector<SoundAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<AZStd::string, SoundAssetRef>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<double, SoundAssetRef>>(); // required to support Map<Number,
                                                                                                  // SoundAssetRef> in Script Canvas

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<SoundAssetRef>(
                        "SoundAssetRef", "A wrapper around MiniAudio SoundAsset to be used as a variable in Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    // m_asset
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoundAssetRef::m_asset, "asset", "")
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, false)
                    ->Attribute(AZ::Edit::Attributes::HideProductFilesInAssetPicker, true)
                    ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "MiniAudio Sound Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SoundAssetRef::OnSpawnAssetChanged);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SoundAssetRef>("SoundAssetRef")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, true)
                ->Attribute(AZ::Script::Attributes::Category, "MiniAudio")
                ->Attribute(AZ::Script::Attributes::Module, "miniaudio")
                ->Constructor()
                ->Method("GetAsset", &SoundAssetRef::GetAsset)
                ->Method("SetAsset", &SoundAssetRef::SetAsset);
        }
    }

    SoundAssetRef::~SoundAssetRef()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    SoundAssetRef::SoundAssetRef(const SoundAssetRef& rhs)
    {
        SetAsset(rhs.m_asset);
    }

    SoundAssetRef::SoundAssetRef(SoundAssetRef&& rhs)
    {
        SetAsset(AZStd::move(rhs.m_asset));
    }

    SoundAssetRef& SoundAssetRef::operator=(const SoundAssetRef& rhs)
    {
        if (this != &rhs)
        {
            SetAsset(rhs.m_asset);
        }
        return *this;
    }

    SoundAssetRef& SoundAssetRef::operator=(SoundAssetRef&& rhs)
    {
        if (this != &rhs)
        {
            SetAsset(AZStd::move(rhs.m_asset));
        }
        return *this;
    }

    void SoundAssetRef::SetAsset(const AZ::Data::Asset<SoundAsset>& asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_asset = asset;
        if (m_asset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
        }
    }

    AZ::Data::Asset<SoundAsset> SoundAssetRef::GetAsset() const
    {
        return m_asset;
    }

    void SoundAssetRef::OnSpawnAssetChanged()
    {
        SetAsset(m_asset);
    }

    void SoundAssetRef::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_asset = asset;
    }
} // namespace MiniAudio
