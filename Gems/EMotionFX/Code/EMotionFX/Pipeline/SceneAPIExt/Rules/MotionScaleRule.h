#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
