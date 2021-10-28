/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DirectionalLightComponentController.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewProviderBus.h>

namespace AZ
{
    namespace Render
    {
        void DirectionalLightComponentController::Reflect(ReflectContext* context)
        {
            DirectionalLightComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DirectionalLightComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &DirectionalLightComponentController::m_configuration);
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
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

                behaviorContext->EBus<DirectionalLightRequestBus>("DirectionalLightRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Event("GetColor", &DirectionalLightRequestBus::Events::GetColor)
                    ->Event("SetColor", &DirectionalLightRequestBus::Events::SetColor)
                    ->Event("GetIntensity", &DirectionalLightRequestBus::Events::GetIntensity)
                    ->Event("SetIntensity", static_cast<void(DirectionalLightRequestBus::Events::*)(float)>(&DirectionalLightRequestBus::Events::SetIntensity))
                    ->Event("GetAngularDiameter", &DirectionalLightRequestBus::Events::GetAngularDiameter)
                    ->Event("SetAngularDiameter", &DirectionalLightRequestBus::Events::SetAngularDiameter)
                    ->Event("GetShadowmapSize", &DirectionalLightRequestBus::Events::GetShadowmapSize)
                    ->Event("SetShadowmapSize", &DirectionalLightRequestBus::Events::SetShadowmapSize)
                    ->Event("GetCascadeCount", &DirectionalLightRequestBus::Events::GetCascadeCount)
                    ->Event("SetCascadeCount", &DirectionalLightRequestBus::Events::SetCascadeCount)
                    ->Event("GetSplitRatio", &DirectionalLightRequestBus::Events::GetShadowmapFrustumSplitSchemeRatio)
                    ->Event("SetSplitRatio", &DirectionalLightRequestBus::Events::SetShadowmapFrustumSplitSchemeRatio)
                    ->Event("GetCascadeFarDepth", &DirectionalLightRequestBus::Events::GetCascadeFarDepth)
                    ->Event("SetCascadeFarDepth", &DirectionalLightRequestBus::Events::SetCascadeFarDepth)
                    ->Event("GetSplitAutomatic", &DirectionalLightRequestBus::Events::GetShadowmapFrustumSplitAutomatic)
                    ->Event("SetSplitAutomatic", &DirectionalLightRequestBus::Events::SetShadowmapFrustumSplitAutomatic)
                    ->Event("GetCameraEntityId", &DirectionalLightRequestBus::Events::GetCameraEntityId)
                    ->Event("SetCameraEntityId", &DirectionalLightRequestBus::Events::SetCameraEntityId)
                    ->Event("GetShadowFarClipDistance", &DirectionalLightRequestBus::Events::GetShadowFarClipDistance)
                    ->Event("SetShadowFarClipDistance", &DirectionalLightRequestBus::Events::SetShadowFarClipDistance)
                    ->Event("GetGroundHeight", &DirectionalLightRequestBus::Events::GetGroundHeight)
                    ->Event("SetGroundHeight", &DirectionalLightRequestBus::Events::SetGroundHeight)
                    ->Event("GetViewFrustumCorrectionEnabled", &DirectionalLightRequestBus::Events::GetViewFrustumCorrectionEnabled)
                    ->Event("SetViewFrustumCorrectionEnabled", &DirectionalLightRequestBus::Events::SetViewFrustumCorrectionEnabled)
                    ->Event("GetDebugColoringEnabled", &DirectionalLightRequestBus::Events::GetDebugColoringEnabled)
                    ->Event("SetDebugColoringEnabled", &DirectionalLightRequestBus::Events::SetDebugColoringEnabled)
                    ->Event("GetShadowFilterMethod", &DirectionalLightRequestBus::Events::GetShadowFilterMethod)
                    ->Event("SetShadowFilterMethod", &DirectionalLightRequestBus::Events::SetShadowFilterMethod)
                    ->Event("GetFilteringSampleCount", &DirectionalLightRequestBus::Events::GetFilteringSampleCount)
                    ->Event("SetFilteringSampleCount", &DirectionalLightRequestBus::Events::SetFilteringSampleCount)
                    ->Event("GetShadowReceiverPlaneBiasEnabled", &DirectionalLightRequestBus::Events::GetShadowReceiverPlaneBiasEnabled)
                    ->Event("SetShadowReceiverPlaneBiasEnabled", &DirectionalLightRequestBus::Events::SetShadowReceiverPlaneBiasEnabled)
                    ->Event("GetShadowBias", &DirectionalLightRequestBus::Events::GetShadowBias)
                    ->Event("SetShadowBias", &DirectionalLightRequestBus::Events::SetShadowBias)
                    ->Event("GetNormalShadowBias", &DirectionalLightRequestBus::Events::GetNormalShadowBias)
                    ->Event("SetNormalShadowBias", &DirectionalLightRequestBus::Events::SetNormalShadowBias)
                    ->VirtualProperty("Color", "GetColor", "SetColor")
                    ->VirtualProperty("Intensity", "GetIntensity", "SetIntensity")
                    ->VirtualProperty("AngularDiameter", "GetAngularDiameter", "SetAngularDiameter")
                    ->VirtualProperty("ShadowmapSize", "GetShadowmapSize", "SetShadowmapSize")
                    ->VirtualProperty("CascadeCount", "GetCascadeCount", "SetCascadeCount")
                    ->VirtualProperty("SplitRatio", "GetSplitRatio", "SetSplitRatio")
                    ->VirtualProperty("CascadeDepth", "GetCascadeFarDepth", "SetCascadeFarDepth")
                    ->VirtualProperty("SplitAutomatic", "GetSplitAutomatic", "SetSplitAutomatic")
                    ->VirtualProperty("ShadowFarClipDistance", "GetShadowFarClipDistance", "SetShadowFarClipDistance")
                    ->VirtualProperty("GroundHeight", "GetGroundHeight", "SetGroundHeight")
                    ->VirtualProperty("ViewFrustumCorrectionEnabled", "GetViewFrustumCorrectionEnabled", "SetViewFrustumCorrectionEnabled")
                    ->VirtualProperty("DebugColoringEnabled", "GetDebugColoringEnabled", "SetDebugColoringEnabled")
                    ->VirtualProperty("ShadowFilterMethod", "GetShadowFilterMethod", "SetShadowFilterMethod")
                    ->VirtualProperty("FilteringSampleCount", "GetFilteringSampleCount", "SetFilteringSampleCount")
                    ->VirtualProperty("ShadowReceiverPlaneBiasEnabled", "GetShadowReceiverPlaneBiasEnabled", "SetShadowReceiverPlaneBiasEnabled")
                    ->VirtualProperty("ShadowBias", "GetShadowBias", "SetShadowBias")
                    ->VirtualProperty("NormalShadowBias", "GetNormalShadowBias", "SetNormalShadowBias");
                ;
            }
        }

        void DirectionalLightComponentController::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void DirectionalLightComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("DirectionalLightService", 0x5270619f));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleComponent"));
        }

        void DirectionalLightComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("DirectionalLightService", 0x5270619f));
        }

        DirectionalLightComponentController::DirectionalLightComponentController(const DirectionalLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DirectionalLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<DirectionalLightFeatureProcessorInterface>(entityId);
            AZ_Error("DirectionalLightComponentController", m_featureProcessor, "Could not find a DirectionalLightFeatureProcessorInterface on the scene.");

            if (m_featureProcessor)
            {
                m_lightHandle = m_featureProcessor->AcquireLight();

                ApplyConfiguration();

                DirectionalLightRequestBus::Handler::BusConnect(entityId);
                TickBus::Handler::BusConnect();
                TransformNotificationBus::MultiHandler::BusConnect(entityId);
                Camera::CameraNotificationBus::Handler::BusConnect();
            }
        }

        void DirectionalLightComponentController::Deactivate()
        {
            Camera::CameraNotificationBus::Handler::BusDisconnect();
            TransformNotificationBus::MultiHandler::BusDisconnect();
            TickBus::Handler::BusDisconnect();
            DirectionalLightRequestBus::Handler::BusDisconnect();

            if (m_featureProcessor)
            {
                m_featureProcessor->ReleaseLight(m_lightHandle);
            }
            m_entityId.SetInvalid();
        }

        void DirectionalLightComponentController::SetConfiguration(const DirectionalLightComponentConfig& config)
        {
            m_configuration = config;

            if (m_featureProcessor)
            {
                ApplyConfiguration();
            }
        }

        const Color& DirectionalLightComponentController::GetColor() const
        {
            return m_configuration.m_color;
        }

        void DirectionalLightComponentController::SetColor(const Color& color)
        {
            m_configuration.m_color = color;
            ColorIntensityChanged();
        }

        float DirectionalLightComponentController::GetIntensity() const
        {
            return m_configuration.m_intensity;
        }

        void DirectionalLightComponentController::SetIntensity(float intensity, PhotometricUnit unit)
        {
            m_photometricValue.ConvertToPhotometricUnit(unit);
            m_photometricValue.SetIntensity(intensity);
            m_configuration.m_intensityMode = unit;
            m_configuration.m_intensity = intensity;
            ColorIntensityChanged();
        }

        void DirectionalLightComponentController::SetIntensity(float intensity)
        {
            m_photometricValue.SetIntensity(intensity);
            m_configuration.m_intensity = intensity;
            ColorIntensityChanged();
        }

        float DirectionalLightComponentController::GetAngularDiameter() const
        {
            return m_configuration.m_angularDiameter;
        }

        void DirectionalLightComponentController::SetAngularDiameter(float angularDiameter)
        {
            m_configuration.m_angularDiameter = angularDiameter;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetAngularDiameter(m_lightHandle, m_configuration.m_angularDiameter);
            }
        }

        ShadowmapSize DirectionalLightComponentController::GetShadowmapSize() const
        {
            return m_configuration.m_shadowmapSize;
        }

        void DirectionalLightComponentController::SetShadowmapSize(ShadowmapSize size)
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
            if (m_featureProcessor)
            {
                m_featureProcessor->SetShadowmapSize(m_lightHandle, size);
            }
        }

        uint32_t DirectionalLightComponentController::GetCascadeCount() const
        {
            return aznumeric_cast<uint32_t>(m_configuration.m_cascadeCount);
        }

        void DirectionalLightComponentController::SetCascadeCount(uint32_t cascadeCount)
        {
            const uint16_t cascadeCount16 = GetMin(static_cast<uint16_t>(Shadow::MaxNumberOfCascades), GetMax<uint16_t>(1, aznumeric_cast<uint16_t>(cascadeCount)));
            cascadeCount = cascadeCount16;
            m_configuration.m_cascadeCount = cascadeCount16;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetCascadeCount(m_lightHandle, cascadeCount16);
            }
        }

        float DirectionalLightComponentController::GetShadowmapFrustumSplitSchemeRatio() const
        {
            return m_configuration.m_shadowmapFrustumSplitSchemeRatio;
        }

        void DirectionalLightComponentController::SetShadowmapFrustumSplitSchemeRatio(float ratio)
        {
            ratio = GetMin(1.f, GetMax(0.f, ratio));
            m_configuration.m_shadowmapFrustumSplitSchemeRatio = ratio;
            m_configuration.m_isShadowmapFrustumSplitAutomatic = true;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetShadowmapFrustumSplitSchemeRatio(m_lightHandle, ratio);
            }
        }

        const Vector4& DirectionalLightComponentController::GetCascadeFarDepth()
        {
            return m_configuration.m_cascadeFarDepths;
        }

        void DirectionalLightComponentController::SetCascadeFarDepth(const Vector4& farDepth)
        {
            m_configuration.m_cascadeFarDepths = farDepth;
            if (m_featureProcessor)
            {
                for (uint16_t index = 0; index < Shadow::MaxNumberOfCascades; ++index)
                {
                    m_featureProcessor->SetCascadeFarDepth(m_lightHandle, index, farDepth.GetElement(index));
                }
            }
        }

        bool DirectionalLightComponentController::GetShadowmapFrustumSplitAutomatic() const
        {
            return m_configuration.m_isShadowmapFrustumSplitAutomatic;
        }

        void DirectionalLightComponentController::SetShadowmapFrustumSplitAutomatic(bool isAutomatic)
        {
            m_configuration.m_isShadowmapFrustumSplitAutomatic = isAutomatic;
        }

        EntityId DirectionalLightComponentController::GetCameraEntityId() const
        {
            return m_configuration.m_cameraEntityId;
        }

        void DirectionalLightComponentController::SetCameraEntityId(EntityId cameraEntityId)
        {
            if (m_configuration.m_cameraEntityId.IsValid())
            {
                TransformNotificationBus::MultiHandler::BusDisconnect(m_configuration.m_cameraEntityId);
            }
            if (cameraEntityId.IsValid())
            {
                TransformNotificationBus::MultiHandler::BusConnect(cameraEntityId);
            }

            m_configuration.m_cameraEntityId = cameraEntityId;
        }

        float DirectionalLightComponentController::GetShadowFarClipDistance() const
        {
            return m_configuration.m_shadowFarClipDistance;
        }

        void DirectionalLightComponentController::SetShadowFarClipDistance(float farDist)
        {
            m_configuration.m_shadowFarClipDistance = farDist;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetShadowFarClipDistance(m_lightHandle, farDist);
            }
        }

        float DirectionalLightComponentController::GetGroundHeight() const
        {
            return m_configuration.m_groundHeight;
        }

        void DirectionalLightComponentController::SetGroundHeight(float groundHeight)
        {
            m_configuration.m_groundHeight = groundHeight;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetGroundHeight(m_lightHandle, groundHeight);
            }
        }

        bool DirectionalLightComponentController::GetViewFrustumCorrectionEnabled() const
        {
            return m_configuration.m_isCascadeCorrectionEnabled;
        }

        void DirectionalLightComponentController::SetViewFrustumCorrectionEnabled(bool enabled)
        {
            m_configuration.m_isCascadeCorrectionEnabled = enabled;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetViewFrustumCorrectionEnabled(m_lightHandle, enabled);
            }
        }

        bool DirectionalLightComponentController::GetDebugColoringEnabled() const
        {
            return m_configuration.m_isDebugColoringEnabled;
        }

        void DirectionalLightComponentController::SetDebugColoringEnabled(bool enabled)
        {
            m_configuration.m_isDebugColoringEnabled = enabled;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDebugFlags(m_lightHandle, enabled ?
                    DirectionalLightFeatureProcessorInterface::DebugDrawFlags::DebugDrawAll :
                    DirectionalLightFeatureProcessorInterface::DebugDrawFlags::DebugDrawNone
                );
            }
        }

        ShadowFilterMethod DirectionalLightComponentController::GetShadowFilterMethod() const
        {
            return m_configuration.m_shadowFilterMethod;
        }

        void DirectionalLightComponentController::SetShadowFilterMethod(ShadowFilterMethod method)
        {
            if (aznumeric_cast<uint32_t>(method) >= aznumeric_cast<uint32_t>(ShadowFilterMethod::Count))
            {
                method = ShadowFilterMethod::None;
            }
            m_configuration.m_shadowFilterMethod = method;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetShadowFilterMethod(m_lightHandle, method);
            }
        }

        uint32_t DirectionalLightComponentController::GetFilteringSampleCount() const
        {
            return aznumeric_cast<uint32_t>(m_configuration.m_filteringSampleCount);
        }

        void DirectionalLightComponentController::SetShadowBias(float bias) 
        {
            m_configuration.m_shadowBias = bias;
            if (m_featureProcessor) 
            {
                m_featureProcessor->SetShadowBias(m_lightHandle, bias);
            }            
        }

        float DirectionalLightComponentController::GetShadowBias() const 
        {
            return m_configuration.m_shadowBias;
        }

        void DirectionalLightComponentController::SetNormalShadowBias(float bias)
        {
            m_configuration.m_normalShadowBias = bias;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetNormalShadowBias(m_lightHandle, bias);
            }
        }

        float DirectionalLightComponentController::GetNormalShadowBias() const
        {
            return m_configuration.m_normalShadowBias;
        }

        void DirectionalLightComponentController::SetFilteringSampleCount(uint32_t count)
        {
            const uint16_t count16 = GetMin(Shadow::MaxPcfSamplingCount, aznumeric_cast<uint16_t>(count));
            m_configuration.m_filteringSampleCount = count16;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetFilteringSampleCount(m_lightHandle, count16);
            }
        }


        const DirectionalLightComponentConfig& DirectionalLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }
        
        void DirectionalLightComponentController::OnTick(float, ScriptTimePoint)
        {
            if (!m_configuration.m_cameraEntityId.IsValid())
            {
                UpdateCameraTransform();
            }
            if (m_featureProcessor)
            {
                m_featureProcessor->SetCameraConfiguration(m_lightHandle, GetCameraConfiguration());
            }
        }

        void DirectionalLightComponentController::OnTransformChanged(const Transform&, const Transform&)
        {
            const EntityId* currentBusId = TransformNotificationBus::GetCurrentBusId();
            if (!currentBusId)
            {
                AZ_Assert(false, "Cannot get current Bus ID.");
                return;
            }
            if (*currentBusId == m_entityId)
            {
                UpdateLightTransform();
            }
            else if (*currentBusId == m_configuration.m_cameraEntityId)
            {
                UpdateCameraTransform();
            }
        }

        void DirectionalLightComponentController::OnCameraAdded(const AZ::EntityId& cameraId)
        {
            if (cameraId == m_configuration.m_cameraEntityId)
            {
                TransformNotificationBus::MultiHandler::BusConnect(cameraId);
            }
        }

        void DirectionalLightComponentController::OnCameraRemoved(const AZ::EntityId& cameraId)
        {
            if (cameraId == m_configuration.m_cameraEntityId)
            {
                TransformNotificationBus::MultiHandler::BusDisconnect(cameraId);
                m_configuration.m_cameraEntityId.SetInvalid();
            }
        }

        void DirectionalLightComponentController::ApplyConfiguration()
        {
            m_photometricValue = PhotometricValue(m_configuration.m_intensity, m_configuration.m_color, m_configuration.m_intensityMode);

            ColorIntensityChanged();
            SetAngularDiameter(m_configuration.m_angularDiameter);

            SetCameraEntityId(m_configuration.m_cameraEntityId);
            SetCascadeCount(m_configuration.m_cascadeCount);
            if (m_configuration.m_isShadowmapFrustumSplitAutomatic)
            {
                SetShadowmapFrustumSplitSchemeRatio(m_configuration.m_shadowmapFrustumSplitSchemeRatio);
            }
            else
            {
                SetCascadeFarDepth(m_configuration.m_cascadeFarDepths);
            }
            SetShadowmapSize(m_configuration.m_shadowmapSize);
            UpdateLightTransform();
            m_lastCameraTransform = Transform::CreateTranslation(AZ::Vector3(std::numeric_limits<float>::max())); // invalid value
            UpdateCameraTransform();
            if (m_featureProcessor)
            {
                m_featureProcessor->SetCameraConfiguration(m_lightHandle, GetCameraConfiguration());
            }
            SetShadowFarClipDistance(m_configuration.m_shadowFarClipDistance);
            SetGroundHeight(m_configuration.m_groundHeight);
            SetViewFrustumCorrectionEnabled(m_configuration.m_isCascadeCorrectionEnabled);
            SetDebugColoringEnabled(m_configuration.m_isDebugColoringEnabled);
            SetShadowFilterMethod(m_configuration.m_shadowFilterMethod);
            SetShadowBias(m_configuration.m_shadowBias);
            SetNormalShadowBias(m_configuration.m_normalShadowBias);
            SetFilteringSampleCount(m_configuration.m_filteringSampleCount);
            SetShadowReceiverPlaneBiasEnabled(m_configuration.m_receiverPlaneBiasEnabled);

            // [GFX TODO][ATOM-1726] share config for multiple light (e.g., light ID).
            // [GFX TODO][ATOM-2416] adapt to multiple viewports.
        }

        Camera::Configuration DirectionalLightComponentController::GetCameraConfiguration() const
        {
            Camera::Configuration config;
            if (m_configuration.m_cameraEntityId.IsValid())
            {
                Camera::CameraRequestBus::EventResult(
                    config,
                    m_configuration.m_cameraEntityId,
                    &Camera::CameraRequestBus::Events::GetCameraConfiguration);
            }
            else
            {
                Camera::ActiveCameraRequestBus::BroadcastResult(
                    config,
                    &Camera::ActiveCameraRequestBus::Events::GetActiveCameraConfiguration);
            }

            if (config.m_fovRadians == 0.f)
            {
                // When the entity does not have a camera component,
                // the config. get invalid.  Then we use the default config.
                config.m_fovRadians = Constants::HalfPi;
                config.m_nearClipDistance = 0.1f;
                config.m_farClipDistance = 100.f;
                config.m_frustumWidth = 100.f;
                config.m_frustumHeight = 100.f;
            }

            return config;
        }

        void DirectionalLightComponentController::UpdateCameraTransform()
        {
            auto cameraTransform = Transform::CreateIdentity();
            if (m_configuration.m_cameraEntityId.IsValid())
            {
                TransformBus::EventResult(
                    cameraTransform,
                    m_configuration.m_cameraEntityId,
                    &TransformBus::Events::GetWorldTM);
            }
            else
            {
                if (const auto& viewportContext = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetDefaultViewportContext())
                {
                    cameraTransform = viewportContext->GetCameraTransform();
                }

            }
            if (cameraTransform == m_lastCameraTransform)
            {
                return;
            }
            m_lastCameraTransform = cameraTransform;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetCameraTransform(m_lightHandle, cameraTransform);
            }
        }

        void DirectionalLightComponentController::UpdateLightTransform()
        {
            auto lightTransform = Transform::CreateIdentity();
            TransformBus::EventResult(
                lightTransform,
                m_entityId,
                &TransformBus::Events::GetWorldTM);
            if (m_featureProcessor)
            {
                m_featureProcessor->SetDirection(m_lightHandle, lightTransform.GetBasisY());
            }
        }

        void DirectionalLightComponentController::ColorIntensityChanged()
        {
            m_photometricValue.SetChroma(m_configuration.m_color);
            m_photometricValue.SetIntensity(m_configuration.m_intensity);
            if (m_featureProcessor)
            {
                m_featureProcessor->SetRgbIntensity(m_lightHandle, m_photometricValue.GetCombinedRgb<PhotometricUnit::Lux>());
            }
        }

        bool DirectionalLightComponentController::GetShadowReceiverPlaneBiasEnabled() const
        {
            return m_configuration.m_receiverPlaneBiasEnabled;
        }

        void DirectionalLightComponentController::SetShadowReceiverPlaneBiasEnabled(bool enable)
        {
            m_configuration.m_receiverPlaneBiasEnabled = enable;
            m_featureProcessor->SetShadowReceiverPlaneBiasEnabled(m_lightHandle, enable);
        }

    } // namespace Render
} // namespace AZ
