/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <CoreLights/LightDelegateInterface.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AZ
{
    class Color;
    class Transform;

    namespace Render
    {
        //! Delegate for managing light shape specific functionality in the AreaLightComponentController.
        template<typename FeatureProcessorType>
        class LightDelegateBase
            : public LightDelegateInterface
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
            , private TransformNotificationBus::Handler
        {
        public:
            LightDelegateBase(EntityId entityId, bool isVisible);
            virtual ~LightDelegateBase();

            void SetConfig(const AreaLightComponentConfig* config) override;

            // LightDelegateInterface overrides...
            void SetChroma(const Color& chroma) override;
            void SetIntensity(float intensity) override;
            float SetPhotometricUnit(PhotometricUnit unit) override;
            void SetAttenuationRadius(float radius) override;
            const PhotometricValue& GetPhotometricValue() const override { return m_photometricValue; }
            void SetLightEmitsBothDirections([[maybe_unused]] bool lightEmitsBothDirections) override {}
            void SetUseFastApproximation([[maybe_unused]] bool useFastApproximation) override {}
            void SetVisibility(bool visibility) override;
            
            void SetEnableShutters(bool enabled) override { m_shuttersEnabled = enabled; }
            void SetShutterAngles([[maybe_unused]]float innerAngleDegrees, [[maybe_unused]]float outerAngleDegrees) override {}

            void SetEnableShadow(bool enabled) override { m_shadowsEnabled = enabled; }
            void SetShadowBias([[maybe_unused]] float bias) override {}
            void SetShadowmapMaxSize([[maybe_unused]] ShadowmapSize size) override {}
            void SetShadowFilterMethod([[maybe_unused]] ShadowFilterMethod method) override {}
            void SetFilteringSampleCount([[maybe_unused]] uint32_t count) override {}
            void SetEsmExponent([[maybe_unused]] float esmExponent) override {}
            void SetNormalShadowBias([[maybe_unused]] float bias) override {}
            void SetShadowCachingMode([[maybe_unused]] AreaLightComponentConfig::ShadowCachingMode cachingMode) override {}

            void SetAffectsGI([[maybe_unused]] bool affectsGI) override {}
            void SetAffectsGIFactor([[maybe_unused]] float affectsGIFactor) override {}
            void SetLightingChannelMask([[maybe_unused]] uint32_t lightingChannelMask) override;

            void SetGoboTexture([[maybe_unused]] AZ::Data::Instance<AZ::RPI::Image> goboTexture) override {}

        protected:
            void InitBase(EntityId entityId);

            // Trivial getters
            FeatureProcessorType* GetFeatureProcessor() const { return m_featureProcessor; }
            const AreaLightComponentConfig* GetConfig() const { return m_componentConfig; }
            typename FeatureProcessorType::LightHandle GetLightHandle() const { return m_lightHandle; }
            const Transform& GetTransform() const { return m_transform; }
            bool GetShuttersEnabled() { return m_shuttersEnabled; }
            bool GetShadowsEnabled() { return m_shadowsEnabled; }
            virtual void HandleShapeChanged() = 0;

            // ShapeComponentNotificationsBus::Handler overrides...
            void OnShapeChanged(ShapeChangeReasons changeReason) override;
            
            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const Transform& local, const Transform& world) override;

            LmbrCentral::ShapeComponentRequests* m_shapeBus = nullptr;

        private:
            // Computes overall effect transform, taking shape translation offsets into account if applicable
            AZ::Transform ComputeOverallTransform(const Transform& entityTransform);

            FeatureProcessorType* m_featureProcessor = nullptr;
            typename FeatureProcessorType::LightHandle m_lightHandle;
            const AreaLightComponentConfig* m_componentConfig = nullptr;

            Transform m_transform;
            PhotometricValue m_photometricValue;
            bool m_shuttersEnabled = false;
            bool m_shadowsEnabled = false;
        };
    } //  namespace Render
} // namespace AZ

#include <CoreLights/LightDelegateBase.inl>
