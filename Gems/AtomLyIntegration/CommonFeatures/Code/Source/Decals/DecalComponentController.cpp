/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Decals/DecalComponentController.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void DecalComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DecalComponentConfig, ComponentConfig>()
                    ->Version(2)
                    ->Field("Attenuation Angle", &DecalComponentConfig::m_attenuationAngle)
                    ->Field("Opacity", &DecalComponentConfig::m_opacity)
                    ->Field("SortKey", &DecalComponentConfig::m_sortKey)
                    ->Field("Material", &DecalComponentConfig::m_materialAsset)
                ;
            }
        }

        void DecalComponentController::Reflect(ReflectContext* context)
        {
            DecalComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DecalComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DecalComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<DecalRequestBus>("DecalRequestBus")
                    ->Event("GetAttenuationAngle", &DecalRequestBus::Events::GetAttenuationAngle)
                    ->Event("SetAttenuationAngle", &DecalRequestBus::Events::SetAttenuationAngle)
                    ->Event("GetOpacity", &DecalRequestBus::Events::GetOpacity)
                    ->Event("SetOpacity", &DecalRequestBus::Events::SetOpacity)
                    ->Event("SetSortKey", &DecalRequestBus::Events::SetSortKey)
                    ->Event("GetSortKey", &DecalRequestBus::Events::GetSortKey)
                    ->VirtualProperty("AttenuationAngle", "GetAttenuationAngle", "SetAttenuationAngle")
                    ->VirtualProperty("Opacity", "GetOpacity", "SetOpacity")
                    ->VirtualProperty("SortKey", "GetSortKey", "SetSortKey")
                    ->Event("SetMaterial", &DecalRequestBus::Events::SetMaterialAssetId)
                    ->Event("GetMaterial", &DecalRequestBus::Events::GetMaterialAssetId)
                    ->VirtualProperty("Material", "GetMaterial", "SetMaterial")
                ;
            }
        }

        void DecalComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DecalService"));
        }

        void DecalComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("DecalService"));
        }

        void DecalComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("TransformService"));
            dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        DecalComponentController::DecalComponentController(const DecalComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DecalComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<DecalFeatureProcessorInterface>(entityId);
            AZ_Assert(m_featureProcessor, "DecalRenderProxy was unable to find a DecalFeatureProcessor on the entityId provided.");
            if (m_featureProcessor)
            {
                m_handle = m_featureProcessor->AcquireDecal();
            }

            m_cachedNonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(m_cachedNonUniformScale, m_entityId, &AZ::NonUniformScaleRequests::GetScale);
            AZ::NonUniformScaleRequestBus::Event(m_entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent,
                m_nonUniformScaleChangedHandler);

            AZ::Transform local, world;
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::GetLocalAndWorld, local, world);
            OnTransformChanged(local, world);

            TransformNotificationBus::Handler::BusConnect(m_entityId);
            DecalRequestBus::Handler::BusConnect(m_entityId);
            ConfigurationChanged();
        }

        void DecalComponentController::Deactivate()
        {
            DecalRequestBus::Handler::BusDisconnect(m_entityId);
            TransformNotificationBus::Handler::BusDisconnect(m_entityId);
            m_nonUniformScaleChangedHandler.Disconnect();
            if (m_featureProcessor)
            {
                m_featureProcessor->ReleaseDecal(m_handle);
                m_handle.Reset();
            }
        }

        void DecalComponentController::SetConfiguration(const DecalComponentConfig& config)
        {
            m_configuration = config;
            ConfigurationChanged();
        }

        const DecalComponentConfig& DecalComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DecalComponentController::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDecalTransform(m_handle, world, m_cachedNonUniformScale);
            }
        }

        void DecalComponentController::HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale)
        {
            m_cachedNonUniformScale = nonUniformScale;
            if (m_featureProcessor)
            {
                AZ::Transform world = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(world, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
                m_featureProcessor->SetDecalTransform(m_handle, world, nonUniformScale);
            }
        }

        void DecalComponentController::ConfigurationChanged()
        {
            AttenuationAngleChanged();
            OpacityChanged();
            SortKeyChanged();
            MaterialChanged();
        }

        float DecalComponentController::GetAttenuationAngle() const
        {
            return m_configuration.m_attenuationAngle;
        }

        void DecalComponentController::SetAttenuationAngle(float attenuationAngle)
        {
            m_configuration.m_attenuationAngle = attenuationAngle;
            AttenuationAngleChanged();
        }

        float DecalComponentController::GetOpacity() const
        {
            return m_configuration.m_opacity;
        }

        void DecalComponentController::SetOpacity(float opacity)
        {
            m_configuration.m_opacity = opacity;
            OpacityChanged();
        }

        uint8_t DecalComponentController::GetSortKey() const
        {
            return m_configuration.m_sortKey;
        }

        void DecalComponentController::SetSortKey(uint8_t sortKey)
        {
            m_configuration.m_sortKey = sortKey;
            SortKeyChanged();
        }

        void DecalComponentController::AttenuationAngleChanged()
        {
            DecalNotificationBus::Event(m_entityId, &DecalNotifications::OnAttenuationAngleChanged, m_configuration.m_attenuationAngle);
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDecalAttenuationAngle(m_handle, m_configuration.m_attenuationAngle);
            }
        }

        void DecalComponentController::OpacityChanged()
        {
            DecalNotificationBus::Event(m_entityId, &DecalNotifications::OnOpacityChanged, m_configuration.m_opacity);
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDecalOpacity(m_handle, m_configuration.m_opacity);
            }
        }

        void DecalComponentController::SortKeyChanged()
        {
            DecalNotificationBus::Event(m_entityId, &DecalNotifications::OnSortKeyChanged, m_configuration.m_sortKey);
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDecalSortKey(m_handle, m_configuration.m_sortKey);
            }
        }

        void DecalComponentController::SetMaterialAssetId(Data::AssetId id)
        {
            Data::Asset<RPI::MaterialAsset> materialAsset;
            materialAsset.Create(id);

            m_configuration.m_materialAsset = materialAsset;
            MaterialChanged();
        }

        void DecalComponentController::MaterialChanged()
        {
            DecalNotificationBus::Event(m_entityId, &DecalNotifications::OnMaterialChanged, m_configuration.m_materialAsset);

            if (m_featureProcessor)
            {
                m_featureProcessor->SetDecalMaterial(m_handle, m_configuration.m_materialAsset.GetId());
            }
        }

        Data::AssetId DecalComponentController::GetMaterialAssetId() const
        {
            return m_configuration.m_materialAsset.GetId();
        }
    } // namespace Render
} // namespace AZ
