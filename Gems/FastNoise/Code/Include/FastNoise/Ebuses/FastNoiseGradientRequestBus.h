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

#include <AzCore/Component/ComponentBus.h>

#include <External/FastNoise/FastNoise.h>

namespace FastNoiseGem
{
    class FastNoiseGradientRequests
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

        virtual float GetFrequency() const = 0;
        virtual void SetFrequency(float freq) = 0;

        virtual FastNoise::Interp GetInterpolation() const = 0;
        virtual void SetInterpolation(FastNoise::Interp interp) = 0;

        virtual FastNoise::NoiseType GetNoiseType() const = 0;
        virtual void SetNoiseType(FastNoise::NoiseType type) = 0;

        virtual int GetOctaves() const = 0;
        virtual void SetOctaves(int octaves) = 0;

        virtual float GetLacunarity() const = 0;
        virtual void SetLacunarity(float lacunarity) = 0;

        virtual float GetGain() const = 0;
        virtual void SetGain(float gain) = 0;

        virtual FastNoise::FractalType GetFractalType() const = 0;
        virtual void SetFractalType(FastNoise::FractalType type) = 0;
    };

    using FastNoiseGradientRequestBus = AZ::EBus<FastNoiseGradientRequests>;
}