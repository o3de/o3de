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

            AZ::Outcome<Data::StringType, Data::StringType> ProduceOutcome::FailureVE(Data::StringType success, Data::StringType failure)
            {
                return AZ::Failure(failure);
            }

            AZ::Outcome<void, Data::StringType> ProduceOutcome::FailureE(Data::StringType success, Data::StringType failure)
            {
                return AZ::Failure(failure);
            }

            AZ::Outcome<Data::StringType, void> ProduceOutcome::FailureV(Data::StringType success, Data::StringType failure)
            {
                return AZ::Failure();
            }

            AZ::Outcome<void, void> ProduceOutcome::Failure(Data::StringType success, Data::StringType failure)
            {
                return AZ::Failure();
            }

            AZ::Outcome<Data::StringType, Data::StringType> ProduceOutcome::SuccessVE(Data::StringType success, Data::StringType failure)
            {
                return AZ::Success(success);
            }

            AZ::Outcome<void, Data::StringType> ProduceOutcome::SuccessE(Data::StringType success, Data::StringType failure)
            {
                return AZ::Success();
            }

            AZ::Outcome<Data::StringType, void> ProduceOutcome::SuccessV(Data::StringType success, Data::StringType failure)
            {
                return AZ::Success(success);
            }

            AZ::Outcome<void, void> ProduceOutcome::Success(Data::StringType success, Data::StringType failure)
            {
                return AZ::Success();
            }

            void ProduceOutcome::Reflect(AZ::ReflectContext* reflection)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                {
                    serializeContext->Class<ProduceOutcome>()
                        ->Version(0)
                        ;

                    if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZ::Outcome<Data::StringType, Data::StringType>>::GetGenericInfo())
                    {
                        genericClassInfo->Reflect(serializeContext);
                    }

                    if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZ::Outcome<Data::StringType, void>>::GetGenericInfo())
                    {
                        genericClassInfo->Reflect(serializeContext);
                    }

                    if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZ::Outcome<void, Data::StringType>>::GetGenericInfo())
                    {
                        genericClassInfo->Reflect(serializeContext);
                    }

                    if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZ::Outcome<void, void>>::GetGenericInfo())
                    {
                        genericClassInfo->Reflect(serializeContext);
                    }
                }

                if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
                {
                    behaviorContext->Class<ProduceOutcome>("ProduceOutcome")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                        ->Method("FailureVE", &ProduceOutcome::FailureVE)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("FailureE", &ProduceOutcome::FailureE)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("FailureV", &ProduceOutcome::FailureV)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("Failure", &ProduceOutcome::Failure)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("SuccessVE", &ProduceOutcome::SuccessVE)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("SuccessE", &ProduceOutcome::SuccessE)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("SuccessV", &ProduceOutcome::SuccessV)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("Success", &ProduceOutcome::Success)
                            ->Attribute(AZ::ScriptCanvasAttributes::AutoUnpackOutputOutcomeSlots, AZ::AttributeIsValid::IfPresent)
                        ->Method("Reflect_AZStd::array<AZ::Vector3, 2>", [](AZStd::array<AZ::Vector3, 2>& vector) {  return vector.size();  })
                            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ;
                }
            }
        } // namespace Auxiliary
    } // namespace UnitTesting
} // namespace ScriptCanvas
