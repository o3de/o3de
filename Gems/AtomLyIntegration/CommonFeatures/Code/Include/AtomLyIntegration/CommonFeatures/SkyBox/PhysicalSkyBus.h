/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>

namespace AZ
{
    namespace Render
    {
        class PhysicalSkyRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::PhysicalSkyRequests, "{2A3744DD-C0B3-4680-9BF3-41886DEC30A1}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~PhysicalSkyRequests() {}

            // reasonable range is 1 ~ 10
            virtual void SetTurbidity(int turbidity) = 0;
            virtual int GetTurbidity() = 0;

            // reasonable range is 0.1 ~ 2.0
            virtual void SetSunRadiusFactor(float factor) = 0;
            virtual float GetSunRadiusFactor() = 0;

            // reasonable range is -4.0 ~ 11.0 for EV100
            virtual void SetSkyIntensity(float intensity, PhotometricUnit unit) = 0;
            virtual void SetSkyIntensity(float intensity) = 0;
            virtual float GetSkyIntensity(PhotometricUnit unit) = 0;
            virtual float GetSkyIntensity() = 0;

            // reasonable range is -4.0 ~ 16.0 for EV100
            virtual void SetSunIntensity(float intensity, PhotometricUnit unit) = 0;
            virtual void SetSunIntensity(float intensity) = 0;
            virtual float GetSunIntensity(PhotometricUnit unit) = 0;
            virtual float GetSunIntensity() = 0;
        };

        typedef AZ::EBus<PhysicalSkyRequests> PhysicalSkyRequestBus;
    }
}
