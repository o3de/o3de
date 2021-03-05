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
#include <SceneAPIExt/Rules/IMotionScaleRule.h>

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
            class MotionScaleRule
                : public IMotionScaleRule
            {
            public:
                AZ_RTTI(MotionScaleRule, "{5C0B0CD3-5CC8-42D0-99EC-FD5744B11B95}", IMotionScaleRule);
                AZ_CLASS_ALLOCATOR_DECL

                MotionScaleRule();
                ~MotionScaleRule() override = default;

                void SetScaleFactor(float value);

                // IMotionScaleRule overrides
                float GetScaleFactor() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                float m_scaleFactor;
            };
        } // Rule
    } // Pipeline
} // EMotionFX