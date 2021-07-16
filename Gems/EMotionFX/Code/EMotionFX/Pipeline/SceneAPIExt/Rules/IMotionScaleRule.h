#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <AzCore/RTTI/RTTI.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class IMotionScaleRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(IMotionScaleRule, "{EFBCC10B-5406-4BD6-B259-BCCD2A24893D}", AZ::SceneAPI::DataTypes::IRule);

                ~IMotionScaleRule() override = default;

                virtual float GetScaleFactor() const = 0;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
