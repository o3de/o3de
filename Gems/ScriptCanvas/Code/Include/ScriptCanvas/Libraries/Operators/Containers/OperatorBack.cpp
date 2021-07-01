/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorBack.h"

#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorBack::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Back"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorBack::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    // Add the OUTPUT slots, most of the time there will only be one
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(type);
                        slotConfiguration.m_toolTip = "The value at the specified index";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);                        

                        m_outputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
            }

            void OperatorBack::InvokeOperator()
            {
                const SlotSet& slotSets = GetSourceSlots();

                if (!slotSets.empty())
                {
                    SlotId sourceSlotId = (*slotSets.begin());
                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        const Datum* inputKeyDatum = FindDatum(*m_inputSlots.begin());
                        AZ::Outcome<Datum, AZStd::string> valueOutcome = BehaviorContextMethodHelper::CallMethodOnDatumUnpackOutcomeSuccess(*containerDatum, "Back", *inputKeyDatum);
                        if (!valueOutcome.IsSuccess())
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Back on container: %s", valueOutcome.GetError().c_str());
                            return;
                        }

                        if (Data::IsVectorContainerType(containerDatum->GetType()))
                        {
                            PushOutput(valueOutcome.TakeValue(), *GetSlot(*m_outputSlots.begin()));
                        }
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorBack::OnInputSignal(const SlotId& slotId)
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
