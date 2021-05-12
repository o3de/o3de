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
            void SetShadowmapMaxSize(ShadowmapSize size) override;
            void SetShadowFilterMethod(ShadowFilterMethod method) override;
            void SetSofteningBoundaryWidthAngle(float widthInDegrees) override;
            void SetPredictionSampleCount(uint32_t count) override;
            void SetFilteringSampleCount(uint32_t count) override;
            void SetPcfMethod(PcfMethod method) override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            float GetRadius() const;

            LmbrCentral::SphereShapeComponentRequests* m_shapeBus = nullptr;
        };

    } // namespace Render
} // namespace AZ

