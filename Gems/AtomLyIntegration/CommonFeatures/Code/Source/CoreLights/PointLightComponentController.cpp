/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <CoreLights/PointLightComponentController.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void PointLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PointLightComponentConfig, ComponentConfig>()
                    ->Version(2)
                    ->Field("Color", &PointLightComponentConfig::m_color)
                    ->Field("ColorIntensityMode", &PointLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &PointLightComponentConfig::m_intensity)
                    ->Field("AttenuationRadiusMode", &PointLightComponentConfig::m_attenuationRadiusMode)
                    ->Field("AttenuationRadius", &PointLightComponentConfig::m_attenuationRadius)
                    ->Field("BulbRadius", &PointLightComponentConfig::m_bulbRadius)
                    ;
            }
        }

        void PointLightComponentController::Reflect(ReflectContext* context)
        {
            PointLightComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PointLightComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &PointLightComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PointLightRequestBus>("PointLightRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Event("GetAttenuationRadius", &PointLightRequestBus::Events::GetAttenuationRadius)
                    ->Event("SetAttenuationRadius", &PointLightRequestBus::Events::SetAttenuationRadius)
                    ->Event("GetAttenuationRadiusIsAutomatic", &PointLightRequestBus::Events::GetAttenuationRadiusIsAutomatic)
                    ->Event("SetAttenuationRadiusIsAutomatic", &PointLightRequestBus::Events::SetAttenuationRadiusIsAutomatic)
                    ->Event("GetBulbRadius", &PointLightRequestBus::Events::GetBulbRadius)
                    ->Event("SetBulbRadius", &PointLightRequestBus::Events::SetBulbRadius)
                    ->Event("GetColor", &PointLightRequestBus::Events::GetColor)
                    ->Event("SetColor", &PointLightRequestBus::Events::SetColor)
                    ->Event("GetIntensity", &PointLightRequestBus::Events::GetIntensity)
                    ->Event("SetIntensity", static_cast<void(PointLightRequestBus::Events::*)(float)>(&PointLightRequestBus::Events::SetIntensity))
                    ->Event("GetIntensityMode", &PointLightRequestBus::Events::GetIntensityMode)
                    ->Event("ConvertToIntensityMode", &PointLightRequestBus::Events::ConvertToIntensityMode)
                    ->VirtualProperty("AttenuationRadius", "GetAttenuationRadius", "SetAttenuationRadius")
                    ->VirtualProperty("AttenuationRadiusIsAutomatic", "GetAttenuationRadiusIsAutomatic", "SetAttenuationRadiusIsAutomatic")
                    ->VirtualProperty("BulbRadius", "GetBulbRadius", "SetBulbRadius")
                    ->VirtualProperty("Color", "GetColor", "SetColor")
                    ->VirtualProperty("Intensity", "GetIntensity", "SetIntensity")
                    ;
            }
        }

        void PointLightComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PointLightService", 0x951d2403));
        }

        void PointLightComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PointLightService", 0x951d2403));
        }

        PointLightComponentController::PointLightComponentController(const PointLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void PointLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<PointLightFeatureProcessorInterface>(entityId);
            AZ_Error("PointLightComponentController", m_featureProcessor, "Could not find a PointLightFeatureProcessorInterface on the scene.");

            if (m_featureProcessor)
            {
                m_lightHandle = m_featureProcessor->AcquireLight();

                AZ::Vector3 position = Vector3::CreateZero();
                AZ::TransformBus::EventResult(position, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
                m_featureProcessor->SetPosition(m_lightHandle, position);

                AZ::Vector3 scale = AZ::Vector3::CreateOne();
                AZ::TransformBus::EventResult(scale, entityId, &AZ::TransformBus::Events::GetWorldScale);
                m_configuration.m_scale = scale.GetMaxElement();

                TransformNotificationBus::Handler::BusConnect(m_entityId);
                PointLightRequestBus::Handler::BusConnect(m_entityId);
                ConfigurationChanged();
            }
        }

        void PointLightComponentController::Deactivate()
        {
            PointLightRequestBus::Handler::BusDisconnect(m_entityId);
            TransformNotificationBus::Handler::BusDisconnect(m_entityId);

            if (m_featureProcessor)
            {
                m_featureProcessor->ReleaseLight(m_lightHandle);
            }
            m_entityId.SetInvalid();
        }

        void PointLightComponentController::SetConfiguration(const PointLightComponentConfig& config)
        {
            m_configuration = config;
            ConfigurationChanged();
        }

        const PointLightComponentConfig& PointLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void PointLightComponentController::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            m_featureProcessor->SetPosition(m_lightHandle, world.GetTranslation());
            if (m_configuration.m_scale != world.GetScale().GetMaxElement())
            {
                m_configuration.UpdateScale(world.GetScale().GetMaxElement());
                BulbRadiusChanged();
                ColorIntensityChanged();
            }
        }

        void PointLightComponentController::ConfigurationChanged()
        {
            m_configuration.UpdateUnscaledIntensity();
            m_configuration.UpdateUnscaledBulbRadius();

            m_photometricValue = PhotometricValue(m_configuration.m_intensity, m_configuration.m_color, m_configuration.m_intensityMode);
            m_photometricValue.SetArea(m_configuration.GetArea());

            ColorIntensityChanged();
            AttenuationRadiusChanged();
            BulbRadiusChanged();
        }

        void PointLightComponentController::ColorIntensityChanged()
        {
            PointLightNotificationBus::Event(m_entityId, &PointLightNotifications::OnColorOrIntensityChanged, m_configuration.m_color, m_configuration.m_intensity);

            m_photometricValue.SetChroma(m_configuration.m_color);
            m_photometricValue.SetIntensity(m_configuration.m_intensity);
            m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<PhotometricUnit::Candela>());
        }

        void PointLightComponentController::AttenuationRadiusChanged()
        {
            if (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
            {
                AutoCalculateAttenuationRadius();
            }
            PointLightNotificationBus::Event(m_entityId, &PointLightNotifications::OnAttenutationRadiusChanged, m_configuration.m_attenuationRadius);
            m_featureProcessor->SetAttenuationRadius(m_lightHandle, m_configuration.m_attenuationRadius);
        }

        void PointLightComponentController::BulbRadiusChanged()
        {
            m_photometricValue.SetArea(m_configuration.GetArea());
            PointLightNotificationBus::Event(m_entityId, &PointLightNotifications::OnBulbRadiusChanged, m_configuration.m_bulbRadius);
            m_featureProcessor->SetBulbRadius(m_lightHandle, m_configuration.m_bulbRadius);
        }

        void PointLightComponentController::AutoCalculateAttenuationRadius()
        {
            // Get combined intensity luma from m_photometricValue, then calculate the radius at which the irradiance will be equal to cutoffIntensity.
            static const float CutoffIntensity = 0.1f; // Make this configurable later.

            float intensity = m_photometricValue.GetCombinedIntensity(PhotometricUnit::Lumen);
            m_configuration.m_attenuationRadius = sqrt(intensity / CutoffIntensity);
        }

        const Color& PointLightComponentController::GetColor() const
        {
            return m_configuration.m_color;
        }

        void PointLightComponentController::SetColor(const Color& color)
        {
            m_configuration.m_color = color;

            PointLightNotificationBus::Event(m_entityId, &PointLightNotifications::OnColorChanged, color);
            ColorIntensityChanged();
        }

        float PointLightComponentController::GetIntensity() const
        {
            return m_configuration.m_intensity;
        }

        void PointLightComponentController::SetIntensity(float intensity)
        {
            m_configuration.m_intensity = intensity;
            m_configuration.UpdateUnscaledIntensity();

            PointLightNotificationBus::Event(m_entityId, &PointLightNotifications::OnIntensityChanged, intensity);
            ColorIntensityChanged();
        }

        void PointLightComponentController::SetIntensity(float intensity, PhotometricUnit intensityMode)
        {
            m_configuration.m_intensityMode = intensityMode;
            SetIntensity(intensity);
        }

        PhotometricUnit PointLightComponentController::GetIntensityMode() const
        {
            return m_configuration.m_intensityMode;
        }

        void PointLightComponentController::ConvertToIntensityMode(PhotometricUnit intensityMode)
        {
            if (m_configuration.m_intensityMode != intensityMode)
            {
                m_configuration.m_intensityMode = intensityMode;
                m_photometricValue.ConvertToPhotometricUnit(intensityMode);
                m_configuration.m_intensity = m_photometricValue.GetIntensity();
                m_configuration.UpdateUnscaledIntensity();
            }
        }

        float PointLightComponentController::GetAttenuationRadius() const
        {
            return m_configuration.m_attenuationRadius;
        }

        void PointLightComponentController::SetAttenuationRadius(float radius)
        {
            m_configuration.m_attenuationRadius = radius;
            AttenuationRadiusChanged();
        }

        float PointLightComponentController::GetBulbRadius() const
        {
            return m_configuration.m_bulbRadius;
        }

        void PointLightComponentController::SetBulbRadius(float bulbRadius)
        {
            m_configuration.m_bulbRadius = bulbRadius;
            m_configuration.UpdateUnscaledBulbRadius();
            BulbRadiusChanged();
        }

        bool PointLightComponentController::GetAttenuationRadiusIsAutomatic() const
        {
            return (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic);
        }

        void PointLightComponentController::SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode)
        {
            m_configuration.m_attenuationRadiusMode = attenuationRadiusMode;
            if (attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
            {
                AutoCalculateAttenuationRadius();
            }
        }

    } // namespace Render
} // namespace AZ
