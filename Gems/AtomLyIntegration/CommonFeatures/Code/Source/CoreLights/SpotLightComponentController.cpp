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

#include <CoreLights/SpotLightComponentController.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void SpotLightComponentController::Reflect(ReflectContext* context)
        {
            SpotLightComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SpotLightComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &SpotLightComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext
                    ->Enum<static_cast<uint32_t>(ShadowFilterMethod::None)>("ShadowFilterMethod_None")
                    ->Enum<static_cast<uint32_t>(ShadowFilterMethod::Pcf)>("ShadowFilterMethod_PCF")
                    ->Enum<static_cast<uint32_t>(ShadowFilterMethod::Esm)>("ShadowFilterMethod_ESM")
                    ->Enum<static_cast<uint32_t>(ShadowFilterMethod::EsmPcf)>("ShadowFilterMethod_ESM_PCF")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::None)>("ShadowmapSize_None")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size256)>("ShadowmapSize_256")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size512)>("ShadowmapSize_512")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size1024)>("ShadowmapSize_1024")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size2048)>("ShadowmapSize_2045")
                    ;

                behaviorContext->EBus<SpotLightRequestBus>("SpotLightRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Event("GetAttenuationRadius", &SpotLightRequestBus::Events::GetAttenuationRadius)
                    ->Event("SetAttenuationRadius", &SpotLightRequestBus::Events::SetAttenuationRadius)
                    ->Event("GetAttenuationRadiusIsAutomatic", &SpotLightRequestBus::Events::GetAttenuationRadiusIsAutomatic)
                    ->Event("SetAttenuationRadiusIsAutomatic", &SpotLightRequestBus::Events::SetAttenuationRadiusIsAutomatic)
                    ->Event("GetColor", &SpotLightRequestBus::Events::GetColor)
                    ->Event("SetColor", &SpotLightRequestBus::Events::SetColor)
                    ->Event("GetIntensity", &SpotLightRequestBus::Events::GetIntensity)
                    ->Event("SetIntensity", &SpotLightRequestBus::Events::SetIntensity)
                    ->Event("GetBulbRadius", &SpotLightRequestBus::Events::GetBulbRadius)
                    ->Event("SetBulbRadius", &SpotLightRequestBus::Events::SetBulbRadius)
                    ->Event("GetInnerConeAngleInDegrees", &SpotLightRequestBus::Events::GetInnerConeAngleInDegrees)
                    ->Event("SetInnerConeAngleInDegrees", &SpotLightRequestBus::Events::SetInnerConeAngleInDegrees)
                    ->Event("GetOuterConeAngleInDegrees", &SpotLightRequestBus::Events::GetOuterConeAngleInDegrees)
                    ->Event("SetOuterConeAngleInDegrees", &SpotLightRequestBus::Events::SetOuterConeAngleInDegrees)
                    ->Event("GetPenumbraBias", &SpotLightRequestBus::Events::GetPenumbraBias)
                    ->Event("SetPenumbraBias", &SpotLightRequestBus::Events::SetPenumbraBias)
                    ->Event("GetEnableShadow", &SpotLightRequestBus::Events::GetEnableShadow)
                    ->Event("SetEnableShadow", &SpotLightRequestBus::Events::SetEnableShadow)
                    ->Event("GetShadowmapSize", &SpotLightRequestBus::Events::GetShadowmapSize)
                    ->Event("SetShadowmapSize", &SpotLightRequestBus::Events::SetShadowmapSize)
                    ->Event("GetShadowFilterMethod", &SpotLightRequestBus::Events::GetShadowFilterMethod)
                    ->Event("SetShadowFilterMethod", &SpotLightRequestBus::Events::SetShadowFilterMethod)
                    ->Event("GetSofteningBoundaryWidthAngle", &SpotLightRequestBus::Events::GetSofteningBoundaryWidthAngle)
                    ->Event("SetSofteningBoundaryWidthAngle", &SpotLightRequestBus::Events::SetSofteningBoundaryWidthAngle)
                    ->Event("GetPredictionSampleCount", &SpotLightRequestBus::Events::GetPredictionSampleCount)
                    ->Event("SetPredictionSampleCount", &SpotLightRequestBus::Events::SetPredictionSampleCount)
                    ->Event("GetFilteringSampleCount", &SpotLightRequestBus::Events::GetFilteringSampleCount)
                    ->Event("SetFilteringSampleCount", &SpotLightRequestBus::Events::SetFilteringSampleCount)
                    ->VirtualProperty("AttenuationRadius", "GetAttenuationRadius", "SetAttenuationRadius")
                    ->VirtualProperty("AttenuationRadiusIsAutomatic", "GetAttenuationRadiusIsAutomatic", "SetAttenuationRadiusIsAutomatic")
                    ->VirtualProperty("Color", "GetColor", "SetColor")
                    ->VirtualProperty("Intensity", "GetIntensity", "SetIntensity")
                    ->VirtualProperty("InnerConeAngleInDegrees", "GetInnerConeAngleInDegrees", "SetInnerConeAngleInDegrees")
                    ->VirtualProperty("OuterConeAngleInDegrees", "GetOuterConeAngleInDegrees", "SetOuterConeAngleInDegrees")
                    ->VirtualProperty("PenumbraBias", "GetPenumbraBias", "SetPenumbraBias")
                    ->VirtualProperty("EnableShadow", "GetEnableShadow", "SetEnableShadow")
                    ->VirtualProperty("ShadowmapSize", "GetShadowmapSize", "SetShadowmapSize")
                    ->VirtualProperty("ShadowFilterMethod", "GetShadowFilterMethod", "SetShadowFilterMethod")
                    ->VirtualProperty("SofteningBoundaryWidthAngle", "GetSofteningBoundaryWidthAngle", "SetSofteningBoundaryWidthAngle")
                    ->VirtualProperty("PredictionSampleCount", "GetPredictionSampleCount", "SetPredictionSampleCount")
                    ->VirtualProperty("FilteringSampelCount", "GetFilteringSampleCount", "SetFilteringSampleCount")
                    ;
            }
        }

        void SpotLightComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SpotLightService", 0x3ae7d498));
        }

        void SpotLightComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SpotLightService", 0x3ae7d498));
        }

        SpotLightComponentController::SpotLightComponentController(const SpotLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void SpotLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<SpotLightFeatureProcessorInterface>(entityId);
            AZ_Error("SpotLightComponentController", m_featureProcessor, "Could not find a SpotLightFeatureProcessorInterface on the scene.");

            if (m_featureProcessor)
            {
                m_lightHandle = m_featureProcessor->AcquireLight();

                TransformNotificationBus::Handler::BusConnect(m_entityId);
                SpotLightRequestBus::Handler::BusConnect(m_entityId);
                ConfigurationChanged();
            }
        }

        void SpotLightComponentController::Deactivate()
        {
            SpotLightRequestBus::Handler::BusDisconnect(m_entityId);
            TransformNotificationBus::Handler::BusDisconnect(m_entityId);

            if (m_featureProcessor)
            {
                m_featureProcessor->ReleaseLight(m_lightHandle);
            }
            m_entityId.SetInvalid();
        }

        void SpotLightComponentController::SetConfiguration(const SpotLightComponentConfig& config)
        {
            m_configuration = config;
            ConfigurationChanged();
        }

        const SpotLightComponentConfig& SpotLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void SpotLightComponentController::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            m_featureProcessor->SetPosition(m_lightHandle, world.GetTranslation());

            const Vector3 forward = Vector3::CreateAxisY();
            const Vector3 lightDirection = world.TransformVector(forward);
            m_featureProcessor->SetDirection(m_lightHandle, lightDirection);
        }

        void SpotLightComponentController::ConfigurationChanged()
        {
            m_photometricValue = PhotometricValue(m_configuration.m_intensity, m_configuration.m_color, m_configuration.m_intensityMode);
            m_photometricValue.SetEffectiveSolidAngle(PhotometricValue::DirectionalEffectiveSteradians);

            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
            OnTransformChanged({}, worldTM);

            ColorIntensityChanged();
            AttenuationRadiusChanged();
            ConeAnglesChanged();
            PenumbraBiasChanged();
            SetBulbRadius(m_configuration.m_bulbRadius);
            SetEnableShadow(m_configuration.m_enabledShadow);
            SetShadowmapSize(m_configuration.m_shadowmapSize);
            SetShadowFilterMethod(m_configuration.m_shadowFilterMethod);
            SetSofteningBoundaryWidthAngle(m_configuration.m_boundaryWidthInDegrees);
            SetPredictionSampleCount(m_configuration.m_predictionSampleCount);
            SetFilteringSampleCount(m_configuration.m_filteringSampleCount);
        }

        void SpotLightComponentController::ColorIntensityChanged()
        {
            SpotLightNotificationBus::Event(m_entityId, &SpotLightNotifications::OnIntensityChanged, m_configuration.m_intensity);
            SpotLightNotificationBus::Event(m_entityId, &SpotLightNotifications::OnColorChanged, m_configuration.m_color);

            m_photometricValue.SetChroma(m_configuration.m_color);
            m_photometricValue.SetIntensity(m_configuration.m_intensity);
            m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<PhotometricUnit::Candela>());
        }

        void SpotLightComponentController::ConeAnglesChanged()
        {
            if (m_configuration.m_innerConeDegrees > m_configuration.m_outerConeDegrees)
            {
                m_configuration.m_innerConeDegrees = m_configuration.m_outerConeDegrees;
            }

            SpotLightNotificationBus::Event(m_entityId, &SpotLightNotifications::OnConeAnglesChanged, m_configuration.m_innerConeDegrees, m_configuration.m_outerConeDegrees);
            m_featureProcessor->SetConeAngles(m_lightHandle, m_configuration.m_innerConeDegrees, m_configuration.m_outerConeDegrees);
        }

        void SpotLightComponentController::AttenuationRadiusChanged()
        {
            if (m_configuration.m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
            {
                AutoCalculateAttenuationRadius();
            }

            SpotLightNotificationBus::Event(m_entityId, &SpotLightNotifications::OnAttenuationRadiusChanged, m_configuration.m_attenuationRadius);
            m_featureProcessor->SetAttenuationRadius(m_lightHandle, m_configuration.m_attenuationRadius);
        }

        void SpotLightComponentController::PenumbraBiasChanged()
        {
            SpotLightNotificationBus::Event(m_entityId, &SpotLightNotifications::OnPenumbraBiasChanged, m_configuration.m_penumbraBias);
            m_featureProcessor->SetPenumbraBias(m_lightHandle, m_configuration.m_penumbraBias);
        }

        void SpotLightComponentController::AutoCalculateAttenuationRadius()
        {
            // Get combined intensity luma from m_photometricValue, then calculate the radius at which the irradiance will be equal to cutoffIntensity.
            static const float CutoffIntensity = 0.1f; // Make this configurable later.

            float intensity = m_photometricValue.GetCombinedIntensity(PhotometricUnit::Lumen);
            m_configuration.m_attenuationRadius = sqrt(intensity / CutoffIntensity);
        }

        const Color& SpotLightComponentController::GetColor() const
        {
            return m_configuration.m_color;
        }

        void SpotLightComponentController::SetColor(const Color& color)
        {
            m_configuration.m_color = color;
            ColorIntensityChanged();
        }

        float SpotLightComponentController::GetIntensity() const
        {
            return m_configuration.m_intensity;
        }

        void SpotLightComponentController::SetIntensity(float intensity)
        {
            m_configuration.m_intensity = intensity;
            ColorIntensityChanged();
        }
        
        float SpotLightComponentController::GetBulbRadius() const
        {
            return m_configuration.m_bulbRadius;
        }

        void SpotLightComponentController::SetBulbRadius(float bulbRadius)
        {
            m_configuration.m_bulbRadius = bulbRadius;
            m_featureProcessor->SetBulbRadius(m_lightHandle, bulbRadius);
        }

        float SpotLightComponentController::GetInnerConeAngleInDegrees() const
        {
            return m_configuration.m_innerConeDegrees;
        }

        void SpotLightComponentController::SetInnerConeAngleInDegrees(float degrees)
        {
            m_configuration.m_innerConeDegrees = degrees;
            ConeAnglesChanged();
        }

        float SpotLightComponentController::GetOuterConeAngleInDegrees() const
        {
            return m_configuration.m_outerConeDegrees;
        }

        void SpotLightComponentController::SetOuterConeAngleInDegrees(float degrees)
        {
            m_configuration.m_outerConeDegrees = degrees;
            ConeAnglesChanged();
        }

        float SpotLightComponentController::GetPenumbraBias() const
        {
            return m_configuration.m_penumbraBias;
        }

        void SpotLightComponentController::SetPenumbraBias(float penumbraBias)
        {
            m_configuration.m_penumbraBias = penumbraBias;
            PenumbraBiasChanged();
        }

        float SpotLightComponentController::GetAttenuationRadius() const
        {
            return m_configuration.m_attenuationRadius;
        }

        void SpotLightComponentController::SetAttenuationRadius(float radius)
        {
            m_configuration.m_attenuationRadius = radius;
            AttenuationRadiusChanged();
        }

        LightAttenuationRadiusMode SpotLightComponentController::GetAttenuationRadiusMode() const
        {
            return m_configuration.m_attenuationRadiusMode;
        }

        void SpotLightComponentController::SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode)
        {
            m_configuration.m_attenuationRadiusMode = attenuationRadiusMode;
            if (attenuationRadiusMode == LightAttenuationRadiusMode::Automatic)
            {
                AutoCalculateAttenuationRadius();
            }
        }

        bool SpotLightComponentController::GetEnableShadow() const
        {
            return m_configuration.m_enabledShadow;
        }

        void SpotLightComponentController::SetEnableShadow(bool enabled)
        {
            m_configuration.m_enabledShadow = enabled;

            m_featureProcessor->SetShadowmapSize(
                m_lightHandle,
                m_configuration.m_enabledShadow ?
                m_configuration.m_shadowmapSize :
                ShadowmapSize::None);
        }

        ShadowmapSize SpotLightComponentController::GetShadowmapSize() const
        {
            return m_configuration.m_shadowmapSize;
        }

        void SpotLightComponentController::SetShadowmapSize(ShadowmapSize size)
        {
            // The minimum valid ShadowmapSize is 256 and maximum one is 2048.
            const uint32_t sizeInt = aznumeric_cast<uint32_t>(size);
            if (sizeInt < aznumeric_cast<uint32_t>(ShadowmapSize::Size512))
            {
                size = ShadowmapSize::Size256;
            }
            else if (sizeInt < aznumeric_cast<uint32_t>(ShadowmapSize::Size1024))
            {
                size = ShadowmapSize::Size512;
            }
            else if (sizeInt < aznumeric_cast<uint32_t>(ShadowmapSize::Size2048))
            {
                size = ShadowmapSize::Size1024;
            }
            else
            {
                size = ShadowmapSize::Size2048;
            }

            m_configuration.m_shadowmapSize = size;
            m_featureProcessor->SetShadowmapSize(
                m_lightHandle,
                m_configuration.m_enabledShadow ?
                m_configuration.m_shadowmapSize :
                ShadowmapSize::None);
        }

        ShadowFilterMethod SpotLightComponentController::GetShadowFilterMethod() const
        {
            return m_configuration.m_shadowFilterMethod;
        }

        void SpotLightComponentController::SetShadowFilterMethod(ShadowFilterMethod method)
        {
            m_configuration.m_shadowFilterMethod = method;

            m_featureProcessor->SetShadowFilterMethod(m_lightHandle, method);
        }

        float SpotLightComponentController::GetSofteningBoundaryWidthAngle() const
        {
            return m_configuration.m_boundaryWidthInDegrees;
        }

        void SpotLightComponentController::SetSofteningBoundaryWidthAngle(float width)
        {
            m_configuration.m_boundaryWidthInDegrees = width;

            m_featureProcessor->SetShadowBoundaryWidthAngle(m_lightHandle, width);
        }

        uint32_t SpotLightComponentController::GetPredictionSampleCount() const
        {
            return m_configuration.m_predictionSampleCount;
        }

        void SpotLightComponentController::SetPredictionSampleCount(uint32_t count)
        {
            m_configuration.m_predictionSampleCount = count;

            m_featureProcessor->SetPredictionSampleCount(m_lightHandle, count);
        }

        uint32_t SpotLightComponentController::GetFilteringSampleCount() const
        {
            return m_configuration.m_filteringSampleCount;
        }

        void SpotLightComponentController::SetFilteringSampleCount(uint32_t count)
        {
            m_configuration.m_filteringSampleCount = count;

            m_featureProcessor->SetFilteringSampleCount(m_lightHandle, count);
        }

    } // namespace Render
} // namespace AZ
