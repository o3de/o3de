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
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathUtils.h>

// Water level unknown.
#define WATER_LEVEL_UNKNOWN -1000000.f
#define BOTTOM_LEVEL_UNKNOWN -1000000.f

namespace AZ
{
    /**
    * Numeric constants that the editors should use for the ocean properties
    */
    namespace OceanConstants
    {
        // Ocean Height consts are maintained for backwards compatibility
        // @TODO: Remove height / depth related consts when feature toggle is removed.
        static const float s_HeightMin = -AZ::Constants::MaxFloatBeforePrecisionLoss;
        static const float s_HeightMax = AZ::Constants::MaxFloatBeforePrecisionLoss;
        static const float s_HeightUnknown = WATER_LEVEL_UNKNOWN;
        static const float s_BottomUnknown = BOTTOM_LEVEL_UNKNOWN;
        static const float s_DefaultHeight = 16.0f;

        static const float s_CausticsDistanceAttenMin = 0.0f;
        static const float s_CausticsDistanceAttenDefault = 10.0f;
        static const float s_CausticsDistanceAttenMax = 100.0f;
        static const float s_CausticsDepthMin = 0.0f;
        static const float s_CausticsDepthDefault = 8.0f;
        static const float s_CausticsDepthMax = 100.0f;
        static const float s_CausticsIntensityMin = 0.0f;
        static const float s_CausticsIntensityDefault = 1.0f;
        static const float s_CausticsIntensityMax = 10.0f;
        static const float s_CausticsTilingMin = 0.10f;
        static const float s_CausticsTilingDefault = 2.0f;
        static const float s_CausticsTilingMax = 10.0f;

        static const float s_animationWavesAmountMin = 0.2f;
        static const float s_animationWavesAmountMax = 5.0f;
        static const float s_animationWavesAmountDefault = 0.75f;
        static const float s_animationWavesSizeMin = 0.0f;
        static const float s_animationWavesSizeMax = 3.0f;
        static const float s_animationWavesSizeDefault = 1.25f;
        static const float s_animationWavesSpeedMin = 0.0f;
        static const float s_animationWavesSpeedMax = 5.0f;
        static const float s_animationWavesSpeedDefault = 1.0f;
        static const float s_animationWindDirectionMin = 0.0f;
        static const float s_animationWindDirectionMax = 6.2832f;
        static const float s_animationWindDirectionDefault = 1;
        static const float s_animationWindSpeedMin = 0.0f;
        static const float s_animationWindSpeedMax = 1000.0f;
        static const float s_animationWindSpeedDefault = 40.0f;

        static const AZ::Color s_oceanFogColorDefault((AZ::u8)5, (AZ::u8)36, (AZ::u8)32, (AZ::u8)255);
        static const AZ::Color s_oceanNearFogColorDefault((AZ::u8)1, (AZ::u8)7, (AZ::u8)5, (AZ::u8)255);
        static const float s_oceanFogColorMultiplierDefault = 0.15f;
        static const float s_oceanFogDensityDefault = 0.07f;
        static const float s_OceanFogColorMultiplierMin = 0.0f;
        static const float s_OceanFogColorMultiplierMax = 1.0f;
        static const float s_OceanFogDensityMin = 0.0f;
        static const float s_OceanFogDensityMax = 1.0f;

        static const int s_waterTessellationAmountMin = 10;
        static const int s_waterTessellationAmountMax = 500;
        static const int s_waterTessellationDefault = 85;        

        static const bool s_UseOceanBottom = true;

        static const bool s_GodRaysEnabled = true;
        static const float s_UnderwaterDistortion = 1.0f;

        static const float s_oceanIsVeryFarAway = 1000000.f;
    };

}
