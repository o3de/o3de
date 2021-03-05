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

#include <AzCore/Memory/Memory.h>
#include <SceneAPIExt/Rules/ISkinRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class SkinRule
                : public ISkinRule
            {
            public:
                AZ_RTTI(SkinRule, "{B26E7FC9-86A1-4711-8415-8BE4861C08BA}", ISkinRule);
                AZ_CLASS_ALLOCATOR_DECL

                SkinRule();
                ~SkinRule() override = default;

                AZ::u32 GetMaxWeightsPerVertex() const override;
                float GetWeightThreshold() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                AZ::u32 m_maxWeightsPerVertex;
                float m_weightThreshold;
            };
        } // Rule
    } // Pipeline
} // EMotionFX