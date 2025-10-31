/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <CoreLights/LightDelegateBase.h>

#include <Atom/Feature/CoreLights/SimpleSpotLightFeatureProcessorInterface.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AZ::Render
{
    class SimpleSpotLightDelegate final : public LightDelegateBase<SimpleSpotLightFeatureProcessorInterface>
    {
        using Base = LightDelegateBase<SimpleSpotLightFeatureProcessorInterface>;

    public:
        SimpleSpotLightDelegate(EntityId entityId, bool isVisible);

        // LightDelegateBase overrides...
        float CalculateAttenuationRadius(float lightThreshold) const override;
        void DrawDebugDisplay(
            const Transform& transform,
            const Color& color,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected) const override;
        float GetSurfaceArea() const override;
        float GetEffectiveSolidAngle() const override;
        void SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees) override;
        void SetAffectsGI(bool affectsGI) override;
        void SetAffectsGIFactor(float affectsGIFactor) override;
        Aabb GetLocalVisualizationBounds() const override;
        void SetEnableShadow(bool enabled) override;
        void SetShadowBias(float bias) override;
        void SetShadowmapMaxSize(ShadowmapSize size) override;
        void SetShadowFilterMethod(ShadowFilterMethod method) override;
        void SetFilteringSampleCount(uint32_t count) override;
        void SetEsmExponent(float exponent) override;
        void SetNormalShadowBias(float bias) override;
        void SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode) override;
        void SetGoboTexture(AZ::Data::Instance<AZ::RPI::Image> goboTexture) override;

    private:
        void HandleShapeChanged() override;

        struct ConeVisualizationDimensions
        {
            float m_radius;
            float m_height;
        };

        ConeVisualizationDimensions CalculateConeVisualizationDimensions(float degrees) const;
    };

    inline float SimpleSpotLightDelegate::GetEffectiveSolidAngle() const
    {
        return PhotometricValue::DirectionalEffectiveSteradians;
    }
} // namespace AZ::Render
