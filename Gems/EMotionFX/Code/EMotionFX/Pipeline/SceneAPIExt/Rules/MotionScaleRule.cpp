/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/MotionScaleRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionScaleRule, AZ::SystemAllocator, 0)

            MotionScaleRule::MotionScaleRule()
                : m_scaleFactor(1.0f)
            {
            }

            void MotionScaleRule::SetScaleFactor(float value)
            {
                m_scaleFactor = value;
            }

            float MotionScaleRule::GetScaleFactor()  const
            {
                return m_scaleFactor;
            }

            void MotionScaleRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IMotionScaleRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                
                serializeContext->Class<MotionScaleRule, IMotionScaleRule>()->Version(1)
                    ->Field("scaleFactor", &MotionScaleRule::m_scaleFactor);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionScaleRule>("Scale motion", "Scale the spatial extent of motion")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionScaleRule::m_scaleFactor, "Scale factor", "Scale factor")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f);
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
