/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightBus.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>

namespace AZ
{
    namespace Render
    {
        class EditorDirectionalLightComponent;

        class DirectionalLightComponentController final
            : public DirectionalLightRequestBus::Handler
            , public TickBus::Handler
            , TransformNotificationBus::MultiHandler
            , Camera::CameraNotificationBus::Handler
        {
        public:
            friend class EditorDirectionaLightComponent;

            AZ_TYPE_INFO(AZ::Render::DirectionalLightComponentController, "60A9DFF4-6A05-4D83-81BD-13ADEB95B29C");
            static void Reflect(ReflectContext* context);
            static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            DirectionalLightComponentController() = default;
            DirectionalLightComponentController(const DirectionalLightComponentConfig& config);
            virtual ~DirectionalLightComponentController() = default;

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DirectionalLightComponentConfig& config);
            const DirectionalLightComponentConfig& GetConfiguration() const;

            // DirectionalLightRequestBus::Handler overrides...
            const Color& GetColor() const override;
            void SetColor(const Color& color) override;
            float GetIntensity() const override;
            PhotometricUnit GetIntensityMode() const override;
            void SetIntensityMode(PhotometricUnit unit) override;
            void SetIntensity(float intensity, PhotometricUnit unit) override;
            void SetIntensity(float intensity) override;
            float GetAngularDiameter() const override;
            void SetAngularDiameter(float angularDiameter) override;
            void SetShadowEnabled(bool enable) override;
            bool GetShadowEnabled() const override;
            ShadowmapSize GetShadowmapSize() const override;
            void SetShadowmapSize(ShadowmapSize size) override;
            uint32_t GetCascadeCount() const override;
            void SetCascadeCount(uint32_t cascadeCount) override;
            float GetShadowmapFrustumSplitSchemeRatio() const override;
            void SetShadowmapFrustumSplitSchemeRatio(float ratio) override;
            const Vector4& GetCascadeFarDepth() override;
            void SetCascadeFarDepth(const Vector4& farDepth) override;
            bool GetShadowmapFrustumSplitAutomatic() const override;
            void SetShadowmapFrustumSplitAutomatic(bool isAutomatic) override;
            EntityId GetCameraEntityId() const override;
            void SetCameraEntityId(EntityId entityId) override;
            float GetShadowFarClipDistance() const override;
            void SetShadowFarClipDistance(float farDist) override;
            float GetGroundHeight() const override;
            void SetGroundHeight(float groundHeight) override;
            bool GetViewFrustumCorrectionEnabled() const override;
            void SetViewFrustumCorrectionEnabled(bool enabled) override;
            bool GetDebugColoringEnabled() const override;
            void SetDebugColoringEnabled(bool enabled) override;
            ShadowFilterMethod GetShadowFilterMethod() const override;
            void SetShadowFilterMethod(ShadowFilterMethod method) override;
            uint32_t GetFilteringSampleCount() const override;
            void SetFilteringSampleCount(uint32_t count) override;
            bool GetShadowReceiverPlaneBiasEnabled() const override;
            void SetShadowReceiverPlaneBiasEnabled(bool enable) override;
            float GetShadowBias() const override;
            void SetShadowBias(float bias) override;
            float GetNormalShadowBias() const override;
            void SetNormalShadowBias(float bias) override;            
            bool GetCascadeBlendingEnabled() const override;
            void SetCascadeBlendingEnabled(bool enable) override;
            void SetFullscreenBlurEnabled(bool enable);
            void SetFullscreenBlurConstFalloff(float blurConstFalloff);
            void SetFullscreenBlurDepthFalloffStrength(float blurDepthFalloffStrength);
            bool GetAffectsGI() const override;
            void SetAffectsGI(bool affectsGI) override;
            float GetAffectsGIFactor() const override;
            void SetAffectsGIFactor(float affectsGIFactor) override;
            void BindConfigurationChangedEventHandler(DirectionalLightConfigurationChangedEvent::Handler& configurationChangedHandler) override;
            uint32_t GetLightingChannelMask() const override;
            void SetLightingChannelMask(const uint32_t mask) override;

        private:
            friend class EditorDirectionalLightComponent;

            // TickBus::Handler overrides...
            void OnTick(float deltaTime, ScriptTimePoint time) override;

            // TransformNotificationBus::MultiHandler overrides...
            void OnTransformChanged(const Transform& /*local*/, const Transform& /*world*/) override;

            // Camera::CameraNotificationBus::Handler overrides...
            void OnCameraAdded(const AZ::EntityId& cameraId) override;
            void OnCameraRemoved(const AZ::EntityId& cameraId) override;

            //! This apply the contents of m_configuration to the light.
            void ApplyConfiguration();

            //! This returns the configuration of camera.
            Camera::Configuration GetCameraConfiguration() const;

            //! This updates the current camera transform.
            //! The camera transform defines the camera view frustum.
            void UpdateCameraTransform();

            //! This updates the current directional light transform.
            void UpdateLightTransform();

            //! Updates current directional light color and intensity.
            void ColorIntensityChanged();

            //! Updates light channel mask.
            void LightingChannelMaskChanged();

            DirectionalLightComponentConfig m_configuration;
            EntityId m_entityId{ EntityId::InvalidEntityId };
            Transform m_lastCameraTransform = Transform::CreateTranslation(AZ::Vector3(std::numeric_limits<float>::max()));

            PhotometricValue m_photometricValue;

            DirectionalLightFeatureProcessorInterface* m_featureProcessor = nullptr;
            DirectionalLightFeatureProcessorInterface::LightHandle m_lightHandle;

            //! Event used to signal when at least one of the properties changes.
            DirectionalLightConfigurationChangedEvent m_configurationChangedEvent;
        };

    } // namespace Render
} // namespace AZ
