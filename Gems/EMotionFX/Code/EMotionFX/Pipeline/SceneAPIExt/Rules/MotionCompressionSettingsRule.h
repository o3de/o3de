#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
