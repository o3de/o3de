/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace GradientSignal
{
    class PerlinGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual int GetRandomSeed() const = 0;
        virtual void SetRandomSeed(int seed) = 0;

        virtual int GetOctaves() const = 0;
        virtual void SetOctaves(int octaves) = 0;

        virtual float GetAmplitude() const = 0;
        virtual void SetAmplitude(float amp) = 0;

        virtual float GetFrequency() const = 0;
        virtual void SetFrequency(float frequency) = 0;
    };

    using PerlinGradientRequestBus = AZ::EBus<PerlinGradientRequests>;
}
