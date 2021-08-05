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
            class IMotionCompressionSettingsRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(IMotionCompressionSettingsRule, "{8BECDD88-9940-4E8E-9E5C-4E088F8D0BDC}", AZ::SceneAPI::DataTypes::IRule);

                ~IMotionCompressionSettingsRule() override = default;

                virtual float GetMaxTranslationError() const = 0;
                virtual float GetMaxRotationError() const = 0;
                virtual float GetMaxScaleError() const = 0;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
