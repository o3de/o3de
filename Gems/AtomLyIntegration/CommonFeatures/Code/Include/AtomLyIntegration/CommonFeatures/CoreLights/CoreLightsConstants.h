/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        inline constexpr AZ::TypeId AreaLightComponentTypeId{ "{744B3961-6242-4461-983F-2817D9D29C30}" };
        inline constexpr AZ::TypeId EditorAreaLightComponentTypeId{ "{8B605C0C-9027-4E0B-BA8C-19E396F8F262}" };
        inline constexpr AZ::TypeId DirectionalLightComponentTypeId{ "{13054592-2753-46C2-B19E-59670D4CE03D}" };
        inline constexpr AZ::TypeId EditorDirectionalLightComponentTypeId{ "{45B97527-6E72-411B-BC23-00068CF01580}" };

        enum class LightAttenuationRadiusMode : uint8_t
        {
            Explicit,
            Automatic,
        };

        inline void CoreLightConstantsReflect(ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext
                    ->Enum<(int)LightAttenuationRadiusMode::Automatic>("LightAttenuationRadiusMode_Automatic")
                    ->Enum<(int)LightAttenuationRadiusMode::Explicit>("LightAttenuationRadiusMode_Explicit")
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
