/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Auxiliary.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/AZStdContainers.inl>

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        namespace Auxiliary
        {
            void EBusTraits::Reflect(AZ::ReflectContext* context)
            {
                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->EBus<EBus>("EBus")
                        ->Attribute(AZ::Script::Attributes::Category, "UnitTesting")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Event("CStyleToCStyle", &EBus::Events::CStyleToCStyle)
                        ->Event("CStyleToStringView", &EBus::Events::CStyleToStringView)
                        ->Event("CStyleToString", &EBus::Events::CStyleToString)
                        ->Event("IntOne", &EBus::Events::IntOne)
                        ->Event("IntTwo", &EBus::Events::IntTwo)
                        ->Event("IntThree", &EBus::Events::IntThree)
                        ->Handler<EBusHandler>()
                        ;
                }
            }

            const char* StringConversion::CStyleToCStyle(const char* input)
            {
                return input;
            }

            void StringConversion::Reflect(AZ::ReflectContext* reflection)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                {
                    serializeContext->Class<StringConversion>()
                        ->Version(0)
                        ;
                }

                if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
                {
                    behaviorContext->Class<StringConversion>("StringConversion")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Method("CStyleToCStyle", &StringConversion::CStyleToCStyle)
                        ;
                }
            }

            void TypeExposition::Reflect(AZ::ReflectContext* reflection)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                {
                    serializeContext->Class<TypeExposition>()
                        ->Version(0)
                        ->Field("arrayVector3_2", &TypeExposition::m_arrayVector3_2)
                        ->Field("outcomeVector3Void", &TypeExposition::m_outcomeVector3Void)
                        ;
                }

                if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
                {
                    behaviorContext->Class<TypeExposition>("TypeExposition")
                        ->Method("Reflect_AZStd::array<AZ::Vector3, 2>", [](AZStd::array<AZ::Vector3, 2>& vector) {  return vector.size();  })
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Method("Reflect_AZ::Outcome<AZ::Vector3, void>", [](AZ::Outcome<AZ::Vector3>& vector) { return vector.IsSuccess();  })
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ;
                }
            }
        }
    }
}
