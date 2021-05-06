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

#include <CoreLights/LightDelegateBase.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Atom/Feature/CoreLights/SimpleSpotLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class SimpleSpotLightDelegate final
            : public LightDelegateBase<SimpleSpotLightFeatureProcessorInterface>
        {
            using Base = LightDelegateBase<SimpleSpotLightFeatureProcessorInterface>;

        public:
            SimpleSpotLightDelegate(EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::DirectionalEffectiveSteradians; }
            void SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees) override;

        private:
            virtual void HandleShapeChanged();
        };

    } // namespace Render
} // namespace AZ

