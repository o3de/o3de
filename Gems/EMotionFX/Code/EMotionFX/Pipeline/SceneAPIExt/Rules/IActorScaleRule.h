#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            class IActorScaleRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(IActorScaleRule, "{20B5EF1C-AE6D-4FBE-B35F-45D286725132}", AZ::SceneAPI::DataTypes::IRule);

                ~IActorScaleRule() override = default;

                virtual float GetScaleFactor() const = 0;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
