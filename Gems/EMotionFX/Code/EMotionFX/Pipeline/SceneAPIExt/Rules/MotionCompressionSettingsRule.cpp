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
#include <SceneAPIExt/Rules/MotionCompressionSettingsRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionCompressionSettingsRule, AZ::SystemAllocator)

            void MotionCompressionSettingsRule::SetMaxTranslationError(float value)
            {
                m_maxTranslationError = value;
            }

            void MotionCompressionSettingsRule::SetMaxRotationError(float value)
            {
                m_maxRotationError = value;
            }

            void MotionCompressionSettingsRule::SetMaxScaleError(float value)
            {
                m_maxScaleError = value;
            }

            float MotionCompressionSettingsRule::GetMaxTranslationError() const
            {
                return m_maxTranslationError;
            }
            
            float MotionCompressionSettingsRule::GetMaxRotationError() const
            {
                return m_maxRotationError;
            }
            
            float MotionCompressionSettingsRule::GetMaxScaleError() const
            {
                return m_maxScaleError;
            }

            void MotionCompressionSettingsRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }
                
                serializeContext->Class<IMotionCompressionSettingsRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

                serializeContext->Class<MotionCompressionSettingsRule, IMotionCompressionSettingsRule>()->Version(2)
                    ->Field("maxTranslationError", &MotionCompressionSettingsRule::m_maxTranslationError)
                    ->Field("maxRotationError", &MotionCompressionSettingsRule::m_maxRotationError)
                    ->Field("maxScaleError", &MotionCompressionSettingsRule::m_maxScaleError);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionCompressionSettingsRule>("Compression settings", "Error tolerance settings while compressing")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionCompressionSettingsRule::m_maxTranslationError, "Max translation error tolerance", "Maximum error allowed in translation")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 6)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionCompressionSettingsRule::m_maxRotationError, "Max rotation error tolerance", "Maximum error allowed in rotation")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 6)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionCompressionSettingsRule::m_maxScaleError, "Max scale error tolerance", "Maximum error allowed in scale")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 6);
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
