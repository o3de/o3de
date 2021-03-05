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
#include <SceneAPIExt/Rules/IMotionCompressionSettingsRule.h>

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
            // Deprecated. MotionCompressionSettingsRule become part of the motion sampling rule.
            class MotionCompressionSettingsRule
                : public IMotionCompressionSettingsRule
            {
            public:
                AZ_RTTI(MotionCompressionSettingsRule, "{2717884D-1F28-4E57-91E2-974FD985C075}", IMotionCompressionSettingsRule);
                AZ_CLASS_ALLOCATOR_DECL

                MotionCompressionSettingsRule() = default;
                ~MotionCompressionSettingsRule() override = default;

                void SetMaxTranslationError(float value);
                void SetMaxRotationError(float value);
                void SetMaxScaleError(float value);

                // IMotionCompressionSettingsRule overrides
                float GetMaxTranslationError() const override;
                float GetMaxRotationError() const override;
                float GetMaxScaleError() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                float m_maxTranslationError = 0.0001f;
                float m_maxRotationError = 0.0001f;
                float m_maxScaleError = 0.0001f;
            };
        } // SceneData
    } // SceneAPI
} // AZ
