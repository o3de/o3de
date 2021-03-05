#pragma once

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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class ISkinRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(ISkinRule, "{5496ECAF-B096-4455-AE72-D55C5B675443}", AZ::SceneAPI::DataTypes::IRule);
                
                ~ISkinRule() override = default;

                virtual AZ::u32 GetMaxWeightsPerVertex() const = 0;
                virtual float GetWeightThreshold() const = 0;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX