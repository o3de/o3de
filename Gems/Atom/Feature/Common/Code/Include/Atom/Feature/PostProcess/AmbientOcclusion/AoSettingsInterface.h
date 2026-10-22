/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>

namespace AZ
{
    namespace Render
    {
        namespace Ao
        {
            enum class AoMethodType : uint8_t
            {
                SSAO,
                GTAO,
            };

            enum class GtaoQualityLevel : uint8_t
            {
                SuperLow,
                Low,
                Medium,
                High,
                SuperHigh,
            };
        }


        class AoSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::AoSettingsInterface, "{3316BA94-CCCA-4088-BAC4-91CFA8149533}");

            // Auto-gen virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
