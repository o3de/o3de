/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/LightDelegateBase.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>

#include <Atom/Feature/CoreLights/DiskLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! Manages rendering a disk light through the disk light feature processor and communication with a disk shape bus for the area light component.
        class DiskLightDelegate final
            : public LightDelegateBase<DiskLightFeatureProcessorInterface>
        {
            using Base = LightDelegateBase<DiskLightFeatureProcessorInterface>;

        public:
            DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::DirectionalEffectiveSteradians; }
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;

            void SetEnableShutters(bool enabled) override;
            void SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees) override;

            void SetEnableShadow(bool enabled) override;
            void SetShadowBias(float bias) override;
            void SetShadowmapMaxSize(ShadowmapSize size) override;
            void SetShadowFilterMethod(ShadowFilterMethod method) override;
            void SetFilteringSampleCount(uint32_t count) override;
            void SetEsmExponent(float exponent) override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            float GetRadius() const;

            LmbrCentral::DiskShapeComponentRequests* m_shapeBus = nullptr;
        };

    } // namespace Render
} // namespace AZ

