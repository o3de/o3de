#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

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
            class MotionSamplingRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(MotionSamplingRule, "{3F54310C-0C08-4074-A1CF-A0BBB25C04DF}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                enum class SampleRateMethod : AZ::u8
                {
                    FromSourceScene = 0,
                    Custom = 1
                };

                float GetCustomSampleRate() const;
                void SetCustomSampleRate(float rate);

                AZ::TypeId GetMotionDataTypeId() const;
                void SetMotionDataTypeId(const AZ::TypeId& typeId);

                SampleRateMethod GetSampleRateMethod() const;
                void SetSampleRateMethod(SampleRateMethod method);

                bool GetKeepDuration() const;
                void SetKeepDuration(bool keepDuration);

                void SetTranslationQualityPercentage(float value);
                float GetTranslationQualityPercentage() const;

                void SetRotationQualityPercentage(float value);
                float GetRotationQualityPercentage() const;

                void SetScaleQualityPercentage(float value);
                float GetScaleQualityPercentage() const;

                float GetAllowedSizePercentage() const;
                void SetAllowedSizePercentage(float percentage);

                // Set the quality percentage using the compression error number from the deprecated motion compression rule.
                void SetTranslationQualityByTranslationError(float value);
                void SetRotationQualityByRotationError(float value);
                void SetScaleQualityByScaleError(float value);

                static void Reflect(AZ::ReflectContext* context);

            protected:
                AZ::Crc32 GetVisibilityCustomSampleRate() const;
                AZ::Crc32 GetVisibilityCompressionSettings() const;
                AZ::Crc32 GetVisibilityAllowedSizePercentage() const;
                
                float m_customSampleRate = 60.0f;
                SampleRateMethod m_sampleRateMethod = SampleRateMethod::FromSourceScene;
                AZ::TypeId m_motionDataType = AZ::TypeId::CreateNull();
                bool m_keepDuration = true;

                float m_translationQualityPercentage = 75.0f;
                float m_rotationQualityPercentage = 75.0f;
                float m_scaleQualityPercentage = 75.0f;

                float m_allowedSizePercentage = 15.0f; // Allow 15 percent larger size, in trade for performance (in Automatic mode, so when m_motionDataType is a Null typeId).
            };
        } // Rule
    } // Pipeline
} // EMotionFX
