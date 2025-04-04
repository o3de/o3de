/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/MotionSamplingRule.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionSamplingRule, AZ::SystemAllocator)

            float MotionSamplingRule::GetCustomSampleRate() const
            {
                return m_customSampleRate;
            }

            void MotionSamplingRule::SetCustomSampleRate(float rate)
            {
                m_customSampleRate = rate;
            }

            AZ::TypeId MotionSamplingRule::GetMotionDataTypeId() const
            {
                return m_motionDataType;
            }

            void MotionSamplingRule::SetMotionDataTypeId(const AZ::TypeId& typeId)
            {
                m_motionDataType = typeId;
            }

            MotionSamplingRule::SampleRateMethod MotionSamplingRule::GetSampleRateMethod() const
            {
                return m_sampleRateMethod;
            }

            void MotionSamplingRule::SetSampleRateMethod(SampleRateMethod method)
            {
                m_sampleRateMethod = method;
            }

            bool MotionSamplingRule::GetKeepDuration() const
            {
                return m_keepDuration;
            }

            void MotionSamplingRule::SetKeepDuration(bool keepDuration)
            {
                m_keepDuration = keepDuration;
            }

            void MotionSamplingRule::SetTranslationQualityPercentage(float value)
            {
                m_translationQualityPercentage = value;
            }

            float MotionSamplingRule::GetTranslationQualityPercentage() const
            {
                return m_translationQualityPercentage;
            }

            void MotionSamplingRule::SetRotationQualityPercentage(float value)
            {
                m_rotationQualityPercentage = value;
            }

            float MotionSamplingRule::GetRotationQualityPercentage() const
            {
                return m_rotationQualityPercentage;
            }

            void MotionSamplingRule::SetScaleQualityPercentage(float value)
            {
                m_scaleQualityPercentage = value;
            }

            float MotionSamplingRule::GetScaleQualityPercentage() const
            {
                return m_scaleQualityPercentage;
            }

            void MotionSamplingRule::SetTranslationQualityByTranslationError(float value)
            {
                float percentage = (1.0f - value / 0.0225f) * 100.0f;
                m_translationQualityPercentage = AZ::GetClamp(percentage, 0.0f, 100.0f);
            }

            void MotionSamplingRule::SetRotationQualityByRotationError(float value)
            {
                float percentage = (1.0f - value / 0.0225f) * 100.0f;
                m_rotationQualityPercentage = AZ::GetClamp(percentage, 0.0f, 100.0f);
            }

            void MotionSamplingRule::SetScaleQualityByScaleError(float value)
            {
                float percentage = (1.0f - value / 0.0225f) * 100.0f;
                m_scaleQualityPercentage = AZ::GetClamp(percentage, 0.0f, 100.0f);
            }

            float MotionSamplingRule::GetAllowedSizePercentage() const
            {
                return m_allowedSizePercentage;
            }

            void MotionSamplingRule::SetAllowedSizePercentage(float percentage)
            {
                m_allowedSizePercentage = percentage;
            }

            void MotionSamplingRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MotionSamplingRule, IRule>()->Version(4)
                    ->Field("motionDataType", &MotionSamplingRule::m_motionDataType)
                    ->Field("sampleRateMethod", &MotionSamplingRule::m_sampleRateMethod)
                    ->Field("customSampleRate", &MotionSamplingRule::m_customSampleRate)
                    ->Field("translationQualityPercentage", &MotionSamplingRule::m_translationQualityPercentage)
                    ->Field("rotationQualityPercentage", &MotionSamplingRule::m_rotationQualityPercentage)
                    ->Field("scaleQualityPercentage", &MotionSamplingRule::m_scaleQualityPercentage)
                    ->Field("allowedSizePercentage", &MotionSamplingRule::m_allowedSizePercentage)
                    ->Field("keepDuration", &MotionSamplingRule::m_keepDuration);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionSamplingRule>("Motion sampling", "A collection of settings related to sampling of the motion")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC_CE("MotionData"), &MotionSamplingRule::m_motionDataType, "Motion data type",
                            "The motion data type to use. This defines how the motion data is stored. This can have an effect on performance and memory usage.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MotionSamplingRule::m_sampleRateMethod, "Sample rate", "Either use the Fbx sample rate or use a custom sample rate. The sample rate is automatically limited to the rate from Fbx.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->EnumAttribute(SampleRateMethod::FromSourceScene, "From Source Scene")
                            ->EnumAttribute(SampleRateMethod::Custom, "Custom sample rate")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionSamplingRule::m_keepDuration, "Keep duration", "When enabled this keep the duration the same as the Fbx motion duration, even if no joints are animated. "
                            "When this option is disabled and the motion doesn't animate any joints then the resulting motion will have a duration of zero seconds.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionSamplingRule::m_customSampleRate, "Custom sample rate", "Overwrite the sample rate of the motion, in frames per second.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 240.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " FPS")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MotionSamplingRule::GetVisibilityCustomSampleRate)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionSamplingRule::m_allowedSizePercentage, "Allowed memory overhead (%)",
                            "The percentage of extra memory usage allowed compared to the smallest size. For example a value of 10 means we are allowed 10 percent more memory worst case, in trade for extra performance.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 0)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " Percent")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MotionSamplingRule::GetVisibilityAllowedSizePercentage)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionSamplingRule::m_translationQualityPercentage, "Translation quality (%)", "The percentage of quality for translation. Higher values preserve quality, but increase memory usage.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 0)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " Percent")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MotionSamplingRule::GetVisibilityCompressionSettings)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionSamplingRule::m_rotationQualityPercentage, "Rotation quality (%)", "The percentage of quality for rotation. Higher values preserve quality, but increase memory usage.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 0)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " Percent")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MotionSamplingRule::GetVisibilityCompressionSettings)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionSamplingRule::m_scaleQualityPercentage, "Scale quality (%)", "The percentage of quality for scale. Higher values preserve quality, but increase memory usage.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 0)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " Percent")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MotionSamplingRule::GetVisibilityCompressionSettings);
                }
            }

            AZ::Crc32 MotionSamplingRule::GetVisibilityCustomSampleRate() const
            {
                return m_sampleRateMethod == SampleRateMethod::FromSourceScene ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
            }

            AZ::Crc32 MotionSamplingRule::GetVisibilityAllowedSizePercentage() const
            {
                return m_motionDataType.IsNull() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
            }

            AZ::Crc32 MotionSamplingRule::GetVisibilityCompressionSettings() const
            {
                // We selected the 'Automatic' motion data type.
                if (m_motionDataType.IsNull())
                {
                    return AZ::Edit::PropertyVisibility::Show;
                }

                // Check whether this type supports the compression settings.
                const MotionDataFactory& factory = GetMotionManager().GetMotionDataFactory();
                if (factory.IsRegisteredTypeId(m_motionDataType))
                {
                    const AZ::Outcome<size_t> index = factory.FindRegisteredIndexByTypeId(m_motionDataType);
                    if (index.IsSuccess())
                    {
                        const MotionData* motionData = factory.GetRegistered(index.GetValue());
                        AZ_Assert(motionData, "Expected a valid motion data pointer");
                        return motionData->GetSupportsOptimizeSettings() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
                    }
                }
                return AZ::Edit::PropertyVisibility::Hide;
            }
        } // Rule
    } // Pipeline
} // EMotionFX
