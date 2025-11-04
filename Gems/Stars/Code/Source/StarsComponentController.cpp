/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StarsComponentController.h"
#include <Atom/RPI.Public/Scene.h>
#include <StarsFeatureProcessor.h>

namespace AZ::Render
{
    void StarsComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StarsComponentConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Exposure", &StarsComponentConfig::m_exposure)
                ->Field("RadiusFactor", &StarsComponentConfig::m_radiusFactor)
                ->Field("StarsAsset", &StarsComponentConfig::m_starsAsset)
                ->Field("TwinkleRate", &StarsComponentConfig::m_twinkleRate)
                ;
        }
    }

    void StarsComponentController::Reflect(ReflectContext* context)
    {
        StarsComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StarsComponentController>()
                ->Version(0)
                ->Field("Configuration", &StarsComponentController::m_configuration);
        }
    }

    void StarsComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("StarsService"));
    }

    void StarsComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("StarsService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void StarsComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    StarsComponentController::StarsComponentController(const StarsComponentConfig& config)
        : m_configuration(config)
    {
    }

    void StarsComponentController::Activate(EntityId entityId)
    {
        EnableFeatureProcessor(entityId);

        TransformNotificationBus::Handler::BusConnect(entityId);
    }

    void StarsComponentController::EnableFeatureProcessor(EntityId entityId)
    {
        m_scene = RPI::Scene::GetSceneForEntityId(entityId);
        if (m_scene)
        {
            m_starsFeatureProcessor = m_scene->EnableFeatureProcessor<StarsFeatureProcessor>();
        }

        if (m_starsFeatureProcessor)
        {
            if (m_configuration.m_starsAsset.IsReady())
            {
                UpdateStarsFromAsset(m_configuration.m_starsAsset);
            }
            else
            {
                OnStarsAssetChanged();
            }

            if(auto transformInterface = TransformBus::FindFirstHandler(entityId))
            {
                m_starsFeatureProcessor->SetOrientation(transformInterface->GetWorldRotationQuaternion());
            }
        }

        OnConfigChanged();
    }

    void StarsComponentController::DisableFeatureProcessor()
    {
        if (m_scene && m_starsFeatureProcessor)
        {
            m_scene->DisableFeatureProcessor<StarsFeatureProcessor>();

            m_starsFeatureProcessor = nullptr;
        }
    }

    void StarsComponentController::OnStarsAssetChanged()
    {
        Data::AssetBus::MultiHandler::BusDisconnect();
        if (m_configuration.m_starsAsset.GetId().IsValid())
        {
            Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_starsAsset.GetId());
            m_configuration.m_starsAsset.QueueLoad();
        }
    }

    void StarsComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
    {
        UpdateStarsFromAsset(asset);
    }

    void StarsComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
    {
        UpdateStarsFromAsset(asset);
    }

    void StarsComponentController::UpdateStarsFromAsset(Data::Asset<Data::AssetData> asset)
    {
        if (asset.GetId() != m_configuration.m_starsAsset.GetId())
        {
            return;
        }

        m_configuration.m_starsAsset = asset;

        StarsAsset *starsAsset = asset.GetAs<StarsAsset>();
        if (starsAsset && starsAsset->m_data.size() > StarsAsset::HeaderSize && m_starsFeatureProcessor)
        {
            Star star{ 0 };
            AZStd::array<float,3> position;
            constexpr int verticesPerStar{ 6 };
            const size_t starsAssetSize = starsAsset->m_data.size();
            const size_t starSize = sizeof(star);
            const size_t numStars = (starsAssetSize - StarsAsset::HeaderSize) / starSize;

            AZStd::vector<StarsFeatureProcessorInterface::StarVertex> stars;
            stars.resize(numStars * verticesPerStar);

            IO::MemoryStream stream(starsAsset->m_data.data(), starsAssetSize);

            // skip the header
            stream.Seek(AZ::IO::OffsetType(StarsAsset::HeaderSize), IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

            for (uint32_t i = 0; i < numStars; ++i)
            {
                stream.Read(starSize, &star);
                
                position[0] = -cosf(AZ::DegToRad(star.declination)) * sinf(AZ::DegToRad(star.ascension * 15.0f));
                position[1] = cosf(AZ::DegToRad(star.declination)) * cosf(AZ::DegToRad(star.ascension * 15.0f));
                position[2] = sinf(AZ::DegToRad(star.declination));

                for (int k = 0; k < verticesPerStar; k++)
                {
                    stars[verticesPerStar * i + k].m_position = position;
                    stars[verticesPerStar * i + k].m_color = (star.magnitude << 24) + (star.blue << 16) + (star.green << 8) + star.red;
                }
            }

            m_starsFeatureProcessor->SetStars(stars);
        }
    }

    void StarsComponentController::Deactivate()
    {
        TransformNotificationBus::Handler::BusDisconnect();
        Data::AssetBus::MultiHandler::BusDisconnect();

        DisableFeatureProcessor();
    }

    void StarsComponentController::SetConfiguration(const StarsComponentConfig& config)
    {
        m_configuration = config;
        OnConfigChanged();
    }

    void StarsComponentController::OnConfigChanged()
    {
        if (m_starsFeatureProcessor)
        {
            m_starsFeatureProcessor->SetExposure(m_configuration.m_exposure);
            m_starsFeatureProcessor->SetRadiusFactor(m_configuration.m_radiusFactor);
            m_starsFeatureProcessor->SetTwinkleRate(m_configuration.m_twinkleRate);
        }
    }

    const StarsComponentConfig& StarsComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void StarsComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        if (m_starsFeatureProcessor)
        {
            m_starsFeatureProcessor->SetOrientation(world.GetRotation());
        }
    }
}
