/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StarsComponentController.h"
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/Stars/StarsFeatureProcessorInterface.h>

namespace AZ::Render
{
    void StarsComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StarsComponentConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("IntensityFactor", &StarsComponentConfig::m_intensityFactor)
                ->Field("RadiusFactor", &StarsComponentConfig::m_radiusFactor)
                ->Field("StarsAsset", &StarsComponentConfig::m_starsAsset)
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
        provided.push_back(AZ_CRC("StarsService", 0x8169a709));
    }

    void StarsComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("StarsService", 0x8169a709));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void StarsComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService"));
    }

    StarsComponentController::StarsComponentController(const StarsComponentConfig& config)
        : m_configuration(config)
    {
    }

    void StarsComponentController::Activate(EntityId entityId)
    {
        m_starsFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::StarsFeatureProcessorInterface>(entityId);
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
        }

        if(auto transformInterface = TransformBus::FindFirstHandler(entityId))
        {
            m_starsFeatureProcessor->SetOrientation(transformInterface->GetWorldRotationQuaternion());
        }

        OnConfigChanged();

        TransformNotificationBus::Handler::BusConnect(entityId);
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
        if (asset.GetId() == m_configuration.m_starsAsset.GetId())
        {
            UpdateStarsFromAsset(asset);
        }
    }

    void StarsComponentController::OnAssetReloaded([[maybe_unused]] Data::Asset<Data::AssetData> asset)
    {
        if (asset.GetId() == m_configuration.m_starsAsset.GetId())
        {
            UpdateStarsFromAsset(asset);
        }
    }

    void StarsComponentController::UpdateStarsFromAsset(Data::Asset<Data::AssetData> asset)
    {
        m_configuration.m_starsAsset = asset;

        AZ::Render::StarsAsset *starsAsset = asset.GetAs<AZ::Render::StarsAsset>();
        if (starsAsset && starsAsset->m_data.size() && starsAsset->m_numStars > 0)
        {
            AZ::IO::MemoryStream stream(starsAsset->m_data.data(), starsAsset->m_data.size());

            // skip the header, version and numstars
            stream.Seek(AZ::Render::StarsAsset::HeaderSize, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

            AZStd::vector<AZ::Render::StarsFeatureProcessorInterface::StarVertex> starVertices;
            constexpr int verticesPerStar{ 6 };
            starVertices.resize(starsAsset->m_numStars * verticesPerStar);

            for (uint32_t i = 0; i < starsAsset->m_numStars; ++i)
            {
                float ra(0);
                stream.Read(sizeof(ra),&ra);

                float dec(0);
                stream.Read(sizeof(dec),&dec);

                uint8_t r(0);
                stream.Read(sizeof(r),&r);

                uint8_t g(0);
                stream.Read(sizeof(g),&g);

                uint8_t b(0);
                stream.Read(sizeof(b),&b);

                uint8_t mag(0);
                stream.Read(sizeof(mag),&mag);

                AZStd::array<float,3> v;
                v[0] = -cosf(AZ::DegToRad(dec)) * sinf(AZ::DegToRad(ra * 15.0f));
                v[1] = cosf(AZ::DegToRad(dec)) * cosf(AZ::DegToRad(ra * 15.0f));
                v[2] = sinf(AZ::DegToRad(dec));

                for (int k = 0; k < verticesPerStar; k++)
                {
                    starVertices[6 * i + k].m_position = v;
                    starVertices[6 * i + k].m_color = (mag << 24) + (b << 16) + (g << 8) + r;
                }
            }

            if (m_starsFeatureProcessor)
            {
                m_starsFeatureProcessor->SetStars(starVertices);
                m_starsFeatureProcessor->Enable(m_visible);
            }
        }
    }

    void StarsComponentController::Deactivate()
    {
        TransformNotificationBus::Handler::BusDisconnect();
        Data::AssetBus::MultiHandler::BusDisconnect();

        if (m_starsFeatureProcessor)
        {
            m_starsFeatureProcessor->Enable(false);
            m_starsFeatureProcessor = nullptr;
        }
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
            m_starsFeatureProcessor->SetIntensityFactor(m_configuration.m_intensityFactor);
            m_starsFeatureProcessor->SetRadiusFactor(m_configuration.m_radiusFactor);
        }
    }

    const StarsComponentConfig& StarsComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void StarsComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_starsFeatureProcessor->SetOrientation(world.GetRotation());
    }
}
