/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <MiniAudio/SoundAsset.h>
#include <MiniAudio/SoundAssetRef.h>

#include <miniaudio.h>
#include "SoundAssetHandler.h"

#include <miniaudio.h>
#include <extras/decoders/libvorbis/miniaudio_libvorbis.h>


namespace MiniAudio
{
    AZ::ComponentDescriptor* MiniAudioSystemComponent_CreateDescriptor()
    {
        return MiniAudioSystemComponent::CreateDescriptor();
    }

    AZ::TypeId MiniAudioSystemComponent_GetTypeId()
    {
        return azrtti_typeid<MiniAudioSystemComponent>();
    }

    void MiniAudioSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SoundAsset::Reflect(context);
        SoundAssetRef::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MiniAudioSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MiniAudioSystemComponent>("MiniAudio", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void MiniAudioSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MiniAudioService"));
    }

    void MiniAudioSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MiniAudioService"));
    }

    void MiniAudioSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MiniAudioSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    MiniAudioSystemComponent::MiniAudioSystemComponent()
    {
        if (MiniAudioInterface::Get() == nullptr)
        {
            MiniAudioInterface::Register(this);
        }
    }

    MiniAudioSystemComponent::~MiniAudioSystemComponent()
    {
        if (MiniAudioInterface::Get() == this)
        {
            MiniAudioInterface::Unregister(this);
        }
    }

    void MiniAudioSystemComponent::Init()
    {
    }

    void MiniAudioSystemComponent::Activate()
    {
        m_resourceManager = AZStd::make_unique<ma_resource_manager>();

        ma_resource_manager_config resourceManagerConfig = ma_resource_manager_config_init();
        ma_decoding_backend_vtable* customBackends[] =
        {
            ma_decoding_backend_libvorbis
        };
        resourceManagerConfig.ppCustomDecodingBackendVTables = customBackends;
        resourceManagerConfig.customDecodingBackendCount = AZ_ARRAY_SIZE(customBackends);
        resourceManagerConfig.pCustomDecodingBackendUserData = nullptr; // could pass `this` in, its passed into each call to res man

        ma_result result = ma_resource_manager_init(&resourceManagerConfig, m_resourceManager.get());
        if (result != MA_SUCCESS)
        {
            AZ_Error("MiniAudio", false, "Failed to initialize miniaudio resource manager, error %d", result);
            m_resourceManager.reset();
            return;
        }
        
        // The number of audio output channels cannot be dynamically changed during runtime yet.
        // The engine configuration setting is done here for future reference.
        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.pResourceManager = m_resourceManager.get();
        engineConfig.channels = m_channelCount;
        m_engine = AZStd::make_unique<ma_engine>();
        result = ma_engine_init(&engineConfig, m_engine.get());
        if (result != MA_SUCCESS)
        {
            ma_resource_manager_uninit(m_resourceManager.get());
            m_resourceManager.reset();
            m_engine.reset();
            AZ_Error("MiniAudio", false, "Failed to initialize miniaudio engine, error %d", result);
            return;

        }

        m_engineInitialized = true;

        MiniAudioRequestBus::Handler::BusConnect();

        {
            SoundAssetHandler* handler = aznew SoundAssetHandler();
            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<SoundAsset>::Uuid());
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, SoundAsset::FileExtension);
            m_assetHandlers.emplace_back(handler);
        }
    }

    void MiniAudioSystemComponent::Deactivate()
    {
        m_assetHandlers.clear();

        if (m_engineInitialized)
        {
            ma_engine_uninit(m_engine.get());
            ma_resource_manager_uninit(m_resourceManager.get());
            m_engine.reset();
            m_resourceManager.reset();
        }
        m_engineInitialized = false;
        MiniAudioRequestBus::Handler::BusDisconnect();
    }

    ma_engine* MiniAudioSystemComponent::GetSoundEngine()
    {
        return m_engine.get();
    }

    void MiniAudioSystemComponent::SetGlobalVolume(float scale)
    {
        m_globalVolume = scale;
        ma_engine_set_volume(m_engine.get(), m_globalVolume);
    }

    float MiniAudioSystemComponent::GetGlobalVolume() const
    {
        return m_globalVolume;
    }

    void MiniAudioSystemComponent::SetGlobalVolumeInDecibels(float decibels)
    {
        m_globalVolume = ma_volume_db_to_linear(decibels);
        SetGlobalVolume(m_globalVolume);
    }

    AZ::u32 MiniAudioSystemComponent::GetChannelCount() const
    {
        return ma_engine_get_channels(m_engine.get());
    }
} // namespace MiniAudio
