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
        public:
            DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible);

            // LightDelegateBase overrides...
            void SetLightEmitsBothDirections(bool lightEmitsBothDirections) override;
            float GetSurfaceArea() const override;
            float GetEffectiveSolidAngle() const override { return PhotometricValue::DirectionalEffectiveSteradians; }
            float CalculateAttenuationRadius(float lightThreshold) const override;
            void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const override;

        private:

            // LightDelegateBase overrides...
            void HandleShapeChanged() override;

            float GetRadius() const;

            LmbrCentral::DiskShapeComponentRequests* m_shapeBus = nullptr;
        };

    } // namespace Render
} // namespace AZ

