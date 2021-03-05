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

#include "OperatorErase.h"

#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorErase::OnInit()
            {
                // Version Conversion away from Operator Base
                if (HasSlots())
                {
                    const Slot* slot = GetSlot(OperatorEraseProperty::GetElementNotFoundSlotId(this));
                    if (slot == nullptr)
                    {
                        ConfigureSlots();
                    }
                }
                ////

                OperatorBase::OnInit();
            }

            void OperatorErase::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Erase"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorErase::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = "Index";
                    slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.SetType(Data::Type::Number());

                    // Add the INDEX as the INPUT slot
                    m_inputSlots.insert(AddSlot(slotConfiguration));
                }
                else if (Data::IsMapContainerType(GetSourceAZType()))
                {
                    AZStd::vector<AZ::Uuid> types = ScriptCanvas::Data::GetContainedTypes(GetSourceAZType());

                    Data::Type type = Data::FromAZType(types[0]);

                    // Only add the KEY as INPUT slot
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = Data::GetName(type);
                    slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.SetType(type);

                    m_inputSlots.insert(AddSlot(slotConfiguration));
                }
            }

            void OperatorErase::InvokeOperator()
            {
                Slot* inputSlot = GetFirstInputSourceSlot();
                Slot* outputSlot = GetFirstOutputSourceSlot();

                if (inputSlot && outputSlot)
                {
                    SlotId sourceSlotId = inputSlot->GetId();
                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        const Datum* inputKeyDatum = FindDatum(*m_inputSlots.begin());
                        AZ::Outcome<Datum, AZStd::string> valueOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Erase", *inputKeyDatum);

                        if (valueOutcome.IsSuccess())
                        {
                            // Push the source container as an output to support chaining
                            PushOutput(*containerDatum, (*outputSlot));

                            auto outcomeInnerResult = valueOutcome.GetValue().GetAs< AZ::Outcome<void, void>>();
                            if (outcomeInnerResult && !outcomeInnerResult->IsSuccess())
                            {
                                SignalOutput(GetSlotId("Element Not Found"));
                                return;
                            }
                        }
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorErase::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    InvokeOperator();
                }
            }
        }
    }
}
