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
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

#include <Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! Manages rendering a capsule light through the capsule light feature processor and communication with a capsule shape bus for the area light component.
        class CapsuleLightDelegate final
            : public LightDelegateBase<CapsuleLightFeatureProcessorInterface>
        {
        public:
            CapsuleLightDelegate(LmbrCentral::CapsuleShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::OmnidirectionalSteradians; }
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;
            void SetAffectsGI(bool affectsGI) override;
            void SetAffectsGIFactor(float affectsGIFactor) override;
            AZ::Aabb GetLocalVisualizationBounds() const override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            // Gets the height of the capsule shape without caps
            float GetInteriorHeight() const;

            struct CapsuleVisualizationDimensions
            {
                float m_radius;
                float m_height;
            };

            CapsuleVisualizationDimensions CalculateCapsuleVisualizationDimensions() const;

            LmbrCentral::CapsuleShapeComponentRequests* m_shapeBus = nullptr;
        };

        inline CapsuleLightDelegate::CapsuleVisualizationDimensions CapsuleLightDelegate::CalculateCapsuleVisualizationDimensions() const
        {
            // Attenuation radius shape is just a capsule with the same internal height, but a radius of the attenuation radius.
            const float radius = GetConfig()->m_attenuationRadius;

            // Add on the caps for the attenuation radius
            const float scale = GetTransform().GetUniformScale();
            const float height = m_shapeBus->GetHeight() * scale;

            return CapsuleLightDelegate::CapsuleVisualizationDimensions{ radius, height };
        }

    } // namespace Render
} // namespace AZ

