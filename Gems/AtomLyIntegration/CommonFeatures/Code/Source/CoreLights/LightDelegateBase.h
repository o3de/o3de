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

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
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

            // LightDelegateInterface overrides...
            void SetChroma(const AZ::Color& chroma) override;
            void SetIntensity(float intensity) override;
            float SetPhotometricUnit(PhotometricUnit unit) override;
            void SetAttenuationRadius(float radius) override;
            const PhotometricValue& GetPhotometricValue() const override { return m_photometricValue; };
            void SetLightEmitsBothDirections([[maybe_unused]] bool lightEmitsBothDirections) override {};
            void SetUseFastApproximation([[maybe_unused]] bool useFastApproximation) override {};
            void SetVisibility(bool visibility) override;
            
            void SetEnableShutters(bool enabled) override { m_shuttersEnabled = enabled; };
            void SetShutterAngles([[maybe_unused]]float innerAngleDegrees, [[maybe_unused]]float outerAngleDegrees) override {};

            void SetEnableShadow(bool enabled) override { m_shadowsEnabled = enabled; };
            void SetShadowmapMaxSize([[maybe_unused]] ShadowmapSize size) override {};
            void SetShadowFilterMethod([[maybe_unused]] ShadowFilterMethod method) override {};
            void SetSofteningBoundaryWidthAngle([[maybe_unused]] float widthInDegrees) override {};
            void SetPredictionSampleCount([[maybe_unused]] uint32_t count) override {};
            void SetFilteringSampleCount([[maybe_unused]] uint32_t count) override {};
            void SetPcfMethod([[maybe_unused]] PcfMethod method) override {};

        protected:
            void InitBase(EntityId entityId);

            // Trivial getters
            FeatureProcessorType* GetFeatureProcessor() const { return m_featureProcessor; };
            typename FeatureProcessorType::LightHandle GetLightHandle() const { return m_lightHandle; };
            const AZ::Transform& GetTransform() const { return m_transform; };
            bool GetShuttersEnabled() { return m_shuttersEnabled; };
            bool GetShadowsEnabled() { return m_shadowsEnabled; };
            virtual void HandleShapeChanged() = 0;

            // ShapeComponentNotificationsBus::Handler overrides...
            void OnShapeChanged(ShapeChangeReasons changeReason) override;
            
            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        private:
            FeatureProcessorType* m_featureProcessor = nullptr;
            typename FeatureProcessorType::LightHandle m_lightHandle;

            LmbrCentral::ShapeComponentRequests* m_shapeBus;
            AZ::Transform m_transform;
            PhotometricValue m_photometricValue;
            bool m_shuttersEnabled = false;
            bool m_shadowsEnabled = false;
        };
    } //  namespace Render
} // namespace AZ

#include <CoreLights/LightDelegateBase.inl>
