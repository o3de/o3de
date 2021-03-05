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

#include "ExpectLessThan.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void ExpectLessThan::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                auto candidateSlot = GetSlot(GetSlotId("Candidate"));

                if (candidateSlot)
                {
                    if (!candidateSlot->IsDynamicSlot())
                    {
                        candidateSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (candidateSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(candidateSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }

                auto referenceSlot = GetSlot(GetSlotId("Reference"));

                if (referenceSlot)
                {
                    if (!referenceSlot->IsDynamicSlot())
                    {
                        referenceSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (referenceSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(referenceSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }
                ////
            }

            void ExpectLessThan::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
                auto lhs = FindDatum(GetSlotId("Candidate"));
                if (!lhs)
                {
                    return;
                }

                auto rhs = FindDatum(GetSlotId("Reference"));
                if (!rhs)
                {
                    return;
                }

                if (lhs->GetType() != rhs->GetType())
                {
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::AddFailure
                        , "Type mismatch in comparison operator");

                    SignalOutput(GetSlotId("Out"));
                    return;
                }

                const auto report = FindDatum(GetSlotId("Report"))->GetAs<Data::StringType>();

                switch (lhs->GetType().GetType())
                {
                case Data::eType::Number:
                      ScriptCanvas::UnitTesting::Bus::Event
                          ( GetOwningScriptCanvasId()
                          , &ScriptCanvas::UnitTesting::BusTraits::ExpectLessThanNumber
                          , *lhs->GetAs<Data::NumberType>()
                          , *rhs->GetAs<Data::NumberType>()
                          , *report);
                      break;

                case Data::eType::String:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectLessThanString
                        , *lhs->GetAs<Data::StringType>()
                        , *rhs->GetAs<Data::StringType>()
                        , *report);

                    break;
                }

                SignalOutput(GetSlotId("Out"));
            }
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas
