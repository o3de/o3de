/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleComponentController.h"

#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <OpenParticleSystem/ParticleFeatureProcessor.h>

namespace OpenParticle
{

        void ParticleComponentController::Reflect(AZ::ReflectContext* context)
    {
        ParticleComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleComponentController>()->Version(0)->Field(
                "Configuration", &ParticleComponentController::m_configuration);
        }
    }

    void ParticleComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        (void)provided;
    }

    void ParticleComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void ParticleComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    static void FixUpParticleAsset(AZ::Data::Asset<ParticleAsset>& particleAsset)
    {
        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, particleAsset.GetHint().c_str(),
            ParticleAsset::TYPEINFO_Uuid(), false);
        if (assetId != particleAsset.GetId())
        {
            if (assetId.IsValid())
            {
                particleAsset = AZ::Data::Asset<ParticleAsset>{ assetId, ParticleAsset::TYPEINFO_Uuid(), particleAsset.GetHint().c_str() };
                particleAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
            }
            else
            {
                AZ_Error("ParticleComponentController", false, "Failed to find asset id for [%s] ", particleAsset.GetHint().c_str());
            }
        }
    }

    ParticleComponentController::ParticleComponentController(const ParticleComponentConfig& config)
        : m_configuration(config)
    {
        FixUpParticleAsset(m_configuration.m_particleAsset);
    }

    void ParticleComponentController::OnTransformChanged(const AZ::Transform&, const AZ::Transform& world)
    {
        if (m_featureProcessor != nullptr)
        {
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, m_entityId, &AZ::NonUniformScaleRequestBus::Events::GetScale);
            m_featureProcessor->SetTransform(m_particleHandle, world, nonUniformScale);
        }
    }

    void ParticleComponentController::OnNonUniformScaleChange(const AZ::Vector3& nonUniformScale)
    {
        if (m_featureProcessor != nullptr)
        {
            const AZ::Transform& worldTM = m_transformInterface->GetWorldTM();
            m_featureProcessor->SetTransform(m_particleHandle, worldTM, nonUniformScale);
        }
    }

    void ParticleComponentController::SetVisible(bool visible)
    {
        if (m_isVisible == visible)
        {
            return;
        }
        if (m_particleHandle.IsValid())
        {
            m_particleHandle->m_visible = visible;
            m_isVisible = visible;
        }
    }

    void ParticleComponentController::Activate(AZ::EntityId entityId)
    {
        FixUpParticleAsset(m_configuration.m_particleAsset);
        m_entityId = entityId;

        m_transformInterface = AZ::TransformBus::FindFirstHandler(m_entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        AZ::NonUniformScaleRequestBus::Event(
            m_entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);
        ParticleRequestBus::Handler::BusConnect(m_entityId);

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        m_featureProcessor = AZ::RPI::Scene::GetFeatureProcessorForEntity<ParticleFeatureProcessorInterface>(m_entityId);
        Register();
    }

    void ParticleComponentController::Deactivate()
    {
        Deregister();

        m_nonUniformScaleChangedHandler.Disconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
        ParticleRequestBus::Handler::BusDisconnect(m_entityId);

        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void ParticleComponentController::SetConfiguration(const ParticleComponentConfig& config)
    {
        m_configuration = config;
    }

    const ParticleComponentConfig& ParticleComponentController::GetConfiguration() const
    {
        return m_configuration;
    }

    void ParticleComponentController::Play()
    {
        if (!m_particleHandle.IsValid()) {
            AZ_Printf("Particle", "No particle asset found to Play");
            return;
        }
        AZ_Printf("Particle", "Particle Play %s", m_particleHandle->m_particleAsset.GetHint().data());
        m_particleHandle->m_status = ParticleStatus::PLAYING;

    }

    void ParticleComponentController::Pause()
    {
        if (!m_particleHandle.IsValid()) {
            AZ_Printf("Particle", "No particle asset found to PAUSE");
            return;
        }
        AZ_Printf("Particle", "Particle Pause %s", m_particleHandle->m_particleAsset.GetHint().data());
        m_particleHandle->m_status = ParticleStatus::PAUSED;
    }

    void ParticleComponentController::Stop()
    {
        if (!m_particleHandle.IsValid()) {
            AZ_Printf("Particle", "No particle asset found to STOP");
            return;
        }
        AZ_Printf("Particle", "Particle Stop %s", m_particleHandle->m_particleAsset.GetHint().data());
        Deregister();
    }

    void ParticleComponentController::SetVisibility(bool visible)
    {
        AZ_Printf("Particle", "Particle Show %s", m_entityId.ToString().c_str());
        SetVisible(visible);
    }

    bool ParticleComponentController::GetVisibility() const
    {
        AZ_Printf("Particle", "Particle Show %s", m_entityId.ToString().c_str());
        return m_isVisible;
    }

    void ParticleComponentController::SetParticleAsset(AZ::Data::Asset<ParticleAsset> particleAsset, bool inParticleEditor)
    {
        Deregister();
        m_configuration.m_inParticleEditor = inParticleEditor;
        m_configuration.m_particleAsset = particleAsset;
        m_configuration.m_particleAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        Register();
    }

    void ParticleComponentController::SetParticleAssetByPath(AZStd::string path)
    {
        AZStd::string::size_type idx = path.find(".");
        if (idx == AZStd::string::npos)
        {
            path += ".azparticle";
        }

        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, path.c_str(), ParticleAsset::TYPEINFO_Uuid(), false);

        if (assetId.IsValid())
        {
            AZ::Data::Asset<ParticleAsset> particleAsset =
                AZ::Data::Asset<ParticleAsset>{ assetId, ParticleAsset::TYPEINFO_Uuid(), path.c_str() };
            SetParticleAsset(particleAsset, false);
        }
        else
        {
            AZ_Error("ParticleComponentController", false, "Failed to find asset id for [%s] ", path.c_str());
        }
    }

    AZStd::string ParticleComponentController::GetParticleAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_particleAsset.GetId());
        return assetPathString;
    }

    void ParticleComponentController::SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath)
    {
        Deregister();
        if (m_featureProcessor != nullptr && m_configuration.m_particleAsset.GetId().IsValid())
        {
            Register();
            m_featureProcessor->SetMaterialDiffuseMap(m_particleHandle, emitterIndex, mapPath);
        }
    }


    void ParticleComponentController::Register()
    {
        if (m_featureProcessor != nullptr && m_configuration.m_particleAsset.GetId().IsValid())
        {
            auto transform = m_transformInterface != nullptr ? m_transformInterface->GetWorldTM() : AZ::Transform::CreateIdentity();
            m_particleHandle = m_featureProcessor->AcquireParticle(m_entityId, m_configuration, transform);
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, m_entityId, &AZ::NonUniformScaleRequestBus::Events::GetScale);
            m_featureProcessor->SetTransform(m_particleHandle, transform, nonUniformScale);
            m_particleHandle->m_visible = m_isVisible;
            m_particleHandle->m_enable = m_configuration.m_enable;
            if (m_configuration.m_autoPlay) {
                m_particleHandle->m_status = ParticleStatus::PLAYING;
            } else {
                m_particleHandle->m_status = ParticleStatus::LOADED;
            }
        }
    }

    void ParticleComponentController::Deregister()
    {
        if (m_featureProcessor != nullptr && m_particleHandle.IsValid())
        {
            m_featureProcessor->ReleaseParticle(m_particleHandle);
        }
    }

    void ParticleComponentController::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);

        if (assetInfo.m_assetType == ParticleAsset::TYPEINFO_Uuid())
        {
            if (m_configuration.m_particleAsset.GetId() != assetId)
            {
                return;
            }

            AZ::Data::Asset<ParticleAsset> particleAsset =
                AZ::Data::Asset<ParticleAsset>{ assetId, ParticleAsset::TYPEINFO_Uuid(), assetInfo.m_relativePath.data() };
            SetParticleAsset(particleAsset, false);
        }
    }

}