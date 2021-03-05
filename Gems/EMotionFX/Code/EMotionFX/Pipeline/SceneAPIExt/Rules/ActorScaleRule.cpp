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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/ActorScaleRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(ActorScaleRule, AZ::SystemAllocator, 0)

            ActorScaleRule::ActorScaleRule()
                : m_scaleFactor(1.0f)
            {
            }

            void ActorScaleRule::SetScaleFactor(float value)
            {
                m_scaleFactor = value;
            }

            float ActorScaleRule::GetScaleFactor()  const
            {
                return m_scaleFactor;
            }

            void ActorScaleRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IActorScaleRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                
                serializeContext->Class<ActorScaleRule, IActorScaleRule>()->Version(1)
                    ->Field("scaleFactor", &ActorScaleRule::m_scaleFactor);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ActorScaleRule>("Scale actor", "Scale the actor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorScaleRule::m_scaleFactor, "Scale factor", "Set the multiplier to scale geometry.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f);
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
