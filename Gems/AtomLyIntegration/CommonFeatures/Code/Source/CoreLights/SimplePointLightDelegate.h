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
#include <Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class SimplePointLightDelegate final
            : public LightDelegateBase<SimplePointLightFeatureProcessorInterface>
        {
        public:
            SimplePointLightDelegate(EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::OmnidirectionalSteradians; }

        private:
            void HandleShapeChanged() override;
        };

    } // namespace Render
} // namespace AZ

