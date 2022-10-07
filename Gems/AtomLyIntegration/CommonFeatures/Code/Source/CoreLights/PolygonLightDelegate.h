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

#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/Feature/CoreLights/PolygonLightFeatureProcessorInterface.h>

namespace AZ::Render
{
    //! Manages rendering a Polygon light through the Polygon light feature processor and communication with a Polygon shape bus for the
    //! area light component.
    class PolygonLightDelegate final : public LightDelegateBase<PolygonLightFeatureProcessorInterface>
    {
    public:
        PolygonLightDelegate(LmbrCentral::PolygonPrismShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

        // LightDelegateBase overrides...
        void SetLightEmitsBothDirections(bool lightEmitsBothDirections) override;
        float GetSurfaceArea() const override;
        float GetEffectiveSolidAngle() const override;
        float CalculateAttenuationRadius(float lightThreshold) const override;
        void DrawDebugDisplay(
            const Transform& transform,
            const Color& color,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected) const override;
        Aabb GetLocalVisualizationBounds() const override;

    private:
        // LightDelegateBase overrides...
        void HandleShapeChanged() override;

        LmbrCentral::PolygonPrismShapeComponentRequests* m_shapeBus = nullptr;
    };

    inline float PolygonLightDelegate::GetEffectiveSolidAngle() const
    {
        return PhotometricValue::DirectionalEffectiveSteradians;
    }
} // namespace AZ::Render
