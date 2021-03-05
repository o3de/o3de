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

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

#include <Atom/Feature/CoreLights/QuadLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! Manages rendering a quad light through the quad light feature processor and communication with a quad shape bus for the area light component.
        class QuadLightDelegate final
            : public LightDelegateBase<QuadLightFeatureProcessorInterface>
        {
        public:
            QuadLightDelegate(LmbrCentral::QuadShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            void SetLightEmitsBothDirections(bool lightEmitsBothDirections) override;
            void SetUseFastApproximation(bool useFastApproximation) override;
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::DirectionalEffectiveSteradians; }
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            float GetWidth() const;
            float GetHeight() const;

            LmbrCentral::QuadShapeComponentRequests* m_shapeBus = nullptr;
        };

    } // namespace Render
} // namespace AZ

