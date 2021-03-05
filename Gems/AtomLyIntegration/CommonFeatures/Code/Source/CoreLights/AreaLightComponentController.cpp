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

#include <CoreLights/AreaLightComponentController.h>
#include <CoreLights/CapsuleLightDelegate.h>
#include <CoreLights/DiskLightDelegate.h>
#include <CoreLights/PolygonLightDelegate.h>
#include <CoreLights/QuadLightDelegate.h>
#include <CoreLights/SphereLightDelegate.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

namespace AZ
{
    namespace Render
    {
        void AreaLightComponentController::Reflect(ReflectContext* context)
        {
            AreaLightComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<AreaLightComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &AreaLightComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AreaLightRequestBus>("AreaLightRequestBus")
                    ->Event("GetAttenuationRadius", &AreaLightRequestBus::Events::GetAttenuationRadius)
                    ->Event("SetAttenuationRadius", &AreaLightRequestBus::Events::SetAttenuationRadius)
                    ->Event("SetAttenuationRadiusMode", &AreaLightRequestBus::Events::SetAttenuationRadiusMode)
                    ->Event("GetColor", &AreaLightRequestBus::Events::GetColor)
                    ->Event("SetColor", &AreaLightRequestBus::Events::SetColor)
                    ->Event("GetEmitsLightBothDirections", &AreaLightRequestBus::Events::GetLightEmitsBothDirections)
                    ->Event("SetEmitsLightBothDirections", &AreaLightRequestBus::Events::SetLightEmitsBothDirections)
                    ->Event("GetUseFastApproximation", &AreaLightRequestBus::Events::GetUseFastApproximation)
                    ->Event("SetUseFastApproximation", &AreaLightRequestBus::Events::SetUseFastApproximation)
                    ->Event("GetIntensity", &AreaLightRequestBus::Events::GetIntensity)
                    ->Event("SetIntensity", static_cast<void(AreaLightRequestBus::Events::*)(float)>(&AreaLightRequestBus::Events::SetIntensity))
                    ->Event("GetIntensityMode", &AreaLightRequestBus::Events::GetIntensityMode)
                    ->Event("ConvertToIntensityMode", &AreaLightRequestBus::Events::ConvertToIntensityMode)
                    ->VirtualProperty("AttenuationRadius", "GetAttenuationRadius", "SetAttenuationRadius")
                    ->VirtualProperty("Color", "GetColor", "SetColor")
                    ->VirtualProperty("EmitsLightBothDirections", "GetEmitsLightBothDirections", "SetEmitsLightBothDirections")
                    ->VirtualProperty("UseFastApproximation", "GetUseFastApproximation", "SetUseFastApproximation")
                    ->VirtualProperty("Intensity", "GetIntensity", "SetIntensity")
                    ;
            }
        }

        void AreaLightComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AreaLightService"));
        }

        void AreaLightComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AreaLightService"));
        }

        void AreaLightComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AreaLightShapeService"));
        }

        AreaLightComponentController::AreaLightComponentController(const AreaLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void AreaLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            CreateLightShapeDelegate();
            AZ_Warning("AreaLightComponentController", m_lightShapeDelegate, "AreaLightComponentController activated without having required component.");

            // Used to determine if the shape can be double-sided.
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_configuration.m_shapeType, m_entityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);

            AreaLightRequestBus::Handler::BusConnect(m_entityId);

            ConfigurationChanged();
        }

        void AreaLightComponentController::Deactivate()
        {
            AreaLightRequestBus::Handler::BusDisconnect(m_entityId);
            m_lightShapeDelegate.reset();
        }

        void AreaLightComponentController::SetConfiguration(const AreaLightComponentConfig& config)
        {
            m_configuration = config;
            ConfigurationChanged();
        }

        const AreaLightComponentConfig& AreaLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void AreaLightComponentController::SetVisibiliy(bool isVisible)
        {
            m_isVisible = isVisible;
            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->SetVisibility(m_isVisible);
            }
        }

        void AreaLightComponentController::ConfigurationChanged()
        {
            ChromaChanged();
            IntensityChanged();
            AttenuationRadiusChanged();

            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->SetLightEmitsBothDirections(m_configuration.m_lightEmitsBothDirections);
                m_lightShapeDelegate->SetUseFastApproximation(m_configuration.m_useFastApproximation);
            }
        }

        void AreaLightComponentController::IntensityChanged()
        {
            AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnColorOrIntensityChanged, m_configuration.m_color, m_configuration.m_intensity);

            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->SetPhotometricUnit(m_configuration.m_intensityMode);
                m_lightShapeDelegate->SetIntensity(m_configuration.m_intensity);
            }
        }

        void AreaLightComponentController::ChromaChanged()
        {
            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->SetChroma(m_configuration.m_color);
            }
        }

        void AreaLightComponentController::AttenuationRadiusChanged()
        {
            if (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
            {
                AutoCalculateAttenuationRadius();
            }
            AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnAttenutationRadiusChanged, m_configuration.m_attenuationRadius);

            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->SetAttenuationRadius(m_configuration.m_attenuationRadius);
            }
        }

        void AreaLightComponentController::AutoCalculateAttenuationRadius()
        {
            if (m_lightShapeDelegate)
            {
                m_configuration.m_attenuationRadius = m_lightShapeDelegate->CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity);
            }
        }

        const Color& AreaLightComponentController::GetColor() const
        {
            return m_configuration.m_color;
        }

        void AreaLightComponentController::SetColor(const Color& color)
        {
            m_configuration.m_color = color;
            AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnColorChanged, color);
            ChromaChanged();
        }

        bool AreaLightComponentController::GetLightEmitsBothDirections() const
        {
            return m_configuration.m_lightEmitsBothDirections;
        }

        void AreaLightComponentController::SetLightEmitsBothDirections(bool value)
        {
            m_configuration.m_lightEmitsBothDirections = value;
        }

        bool AreaLightComponentController::GetUseFastApproximation() const
        {
            return m_configuration.m_useFastApproximation;
        }

        void AreaLightComponentController::SetUseFastApproximation(bool value)
        {
            m_configuration.m_useFastApproximation = value;
        }

        PhotometricUnit AreaLightComponentController::GetIntensityMode() const
        {
            return m_configuration.m_intensityMode;
        }

        float AreaLightComponentController::GetIntensity() const
        {
            return m_configuration.m_intensity;
        }

        void AreaLightComponentController::SetIntensity(float intensity, PhotometricUnit intensityMode)
        {
            m_configuration.m_intensityMode = intensityMode;
            m_configuration.m_intensity = intensity;

            AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnIntensityChanged, intensity, intensityMode);
            IntensityChanged();
        }

        void AreaLightComponentController::SetIntensity(float intensity)
        {
            m_configuration.m_intensity = intensity;

            AreaLightNotificationBus::Event(m_entityId, &AreaLightNotifications::OnIntensityChanged, intensity, m_configuration.m_intensityMode);
            IntensityChanged();
        }

        float AreaLightComponentController::GetAttenuationRadius() const
        {
            return m_configuration.m_attenuationRadius;
        }

        void AreaLightComponentController::SetAttenuationRadius(float radius)
        {
            m_configuration.m_attenuationRadius = radius;
            m_configuration.m_attenuationRadiusMode = LightAttenuationRadiusMode::Explicit;
            AttenuationRadiusChanged();
        }

        void AreaLightComponentController::SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode)
        {
            m_configuration.m_attenuationRadiusMode = attenuationRadiusMode;
            AttenuationRadiusChanged();
        }

        void AreaLightComponentController::ConvertToIntensityMode(PhotometricUnit intensityMode)
        {
            if (m_lightShapeDelegate && m_lightShapeDelegate->GetPhotometricValue().GetType() != intensityMode)
            {
                m_configuration.m_intensityMode = intensityMode;
                m_configuration.m_intensity = m_lightShapeDelegate->SetPhotometricUnit(intensityMode);
            }
        }

        void AreaLightComponentController::HandleDisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected)
        {
            Transform transform = Transform::CreateIdentity();
            TransformBus::EventResult(transform, m_entityId, &TransformBus::Events::GetWorldTM);
            if (m_lightShapeDelegate)
            {
                m_lightShapeDelegate->DrawDebugDisplay(transform, m_configuration.m_color, debugDisplay, isSelected);
            }
        }

        void AreaLightComponentController::CreateLightShapeDelegate()
        {
            LmbrCentral::SphereShapeComponentRequests* sphereShapeInterface = LmbrCentral::SphereShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            if (sphereShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<SphereLightDelegate>(sphereShapeInterface, m_entityId, m_isVisible);
                return;
            }

            LmbrCentral::DiskShapeComponentRequests* diskShapeInterface = LmbrCentral::DiskShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (diskShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<DiskLightDelegate>(diskShapeInterface, m_entityId, m_isVisible);
                return;
            }

            LmbrCentral::CapsuleShapeComponentRequests* capsuleShapeInterface = LmbrCentral::CapsuleShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            if (capsuleShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<CapsuleLightDelegate>(capsuleShapeInterface, m_entityId, m_isVisible);
                return;
            }

            LmbrCentral::QuadShapeComponentRequests* quadShapeInterface = LmbrCentral::QuadShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (quadShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<QuadLightDelegate>(quadShapeInterface, m_entityId, m_isVisible);
                return;
            }

            LmbrCentral::PolygonPrismShapeComponentRequests* polyPrismShapeInterface = LmbrCentral::PolygonPrismShapeComponentRequestBus::FindFirstHandler(m_entityId);
            if (polyPrismShapeInterface)
            {
                m_lightShapeDelegate = AZStd::make_unique<PolygonLightDelegate>(polyPrismShapeInterface, m_entityId, m_isVisible);
                return;
            }
        }

    } // namespace Render
} // namespace AZ
