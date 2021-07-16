#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkinRule
                : public IRule
            {
            public:
                AZ_RTTI(ISkinRule, "{5496ECAF-B096-4455-AE72-D55C5B675443}", IRule);
                
                ~ISkinRule() override = default;

                virtual AZ::u32 GetMaxWeightsPerVertex() const = 0;
                virtual float GetWeightThreshold() const = 0;
            };
        }
    } 
}
