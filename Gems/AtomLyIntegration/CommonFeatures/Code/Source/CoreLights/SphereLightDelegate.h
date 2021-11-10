/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/LightDelegateBase.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class SphereLightDelegate final
            : public LightDelegateBase<PointLightFeatureProcessorInterface>
        {
            using Base = LightDelegateBase<PointLightFeatureProcessorInterface>;

        public:
            SphereLightDelegate(LmbrCentral::SphereShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::OmnidirectionalSteradians; }
            void SetEnableShadow(bool enabled) override;
            void SetShadowBias(float bias) override;
            void SetShadowmapMaxSize(ShadowmapSize size) override;
            void SetShadowFilterMethod(ShadowFilterMethod method) override;
            void SetFilteringSampleCount(uint32_t count) override;
            void SetEsmExponent(float esmExponent) override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            float GetRadius() const;

            LmbrCentral::SphereShapeComponentRequests* m_shapeBus = nullptr;
        };

    } // namespace Render
} // namespace AZ

