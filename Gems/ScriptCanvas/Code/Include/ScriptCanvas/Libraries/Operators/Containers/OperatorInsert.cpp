/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorInsert.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorInsert::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Insert"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorInsert::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    // Add the INDEX as the INPUT slot
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = "Index";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(Data::Type::Number());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }

                    // Add the INPUT slot for the data to insert
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(type);
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);                        

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
                else
                {
                    // Key
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = "Key";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }

                    // Value
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[1]);
                        DataSlotConfiguration slotConfiguration;
                        slotConfiguration.m_name = "Value";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
            }

            void OperatorInsert::InvokeOperator()
            {
                Slot* inputSlot = GetFirstInputSourceSlot();
                Slot* outputSlot = GetFirstOutputSourceSlot();

                if (inputSlot && outputSlot)
                {
                    SlotId sourceSlotId = inputSlot->GetId();
                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        AZ::BehaviorMethod* method = GetOperatorMethod("Insert");
                        AZ_Assert(method, "The contract must have failed because you should not be able to invoke an operator for a type that does not have the method");

                        AZStd::array<AZ::BehaviorValueParameter, BehaviorContextMethodHelper::MaxCount> params;

                        AZ::BehaviorValueParameter* paramFirst(params.begin());
                        AZ::BehaviorValueParameter* paramIter = paramFirst;

                        if (Data::IsVectorContainerType(GetSourceAZType()))
                        {
                            // Container
                            auto behaviorParameter = method->GetArgument(0);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param0 = containerDatum->ToBehaviorValueParameter(*behaviorParameter);
                            paramIter->Set(param0.GetValue());
                            ++paramIter;

                            // Get the size of the container
                            auto sizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Size");
                            if (!sizeOutcome)
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of container: %s", sizeOutcome.GetError().c_str());
                                return;
                            }

                            auto inputSlotIterator = m_inputSlots.begin();

                            // Index
                            behaviorParameter = method->GetArgument(1);
                            const Datum* indexDatum = FindDatum(*inputSlotIterator);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param2 = indexDatum->ToBehaviorValueParameter(*behaviorParameter);

                            paramIter->Set(param2.GetValue());
                            ++paramIter;

                            // Value to insert
                            behaviorParameter = method->GetArgument(2);
                            ++inputSlotIterator;
                            const Datum* inputDatum = FindDatum(*inputSlotIterator);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param1 = inputDatum->ToBehaviorValueParameter(*behaviorParameter);
                            paramIter->Set(param1.GetValue());
                            ++paramIter;
                        }
                        else if (Data::IsMapContainerType(GetSourceAZType()))
                        {
                            auto inputSlotIterator = m_inputSlots.begin();
                            const Datum* keyDatum = FindDatum(*inputSlotIterator++);
                            const Datum* valueDatum = FindDatum(*inputSlotIterator);

                            if (keyDatum && valueDatum)
                            {
                                // Container
                                const AZ::BehaviorParameter* behaviorParameter = method->GetArgument(0);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param0 = containerDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param0.GetValue());
                                ++paramIter;

                                // Key
                                behaviorParameter = method->GetArgument(1);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param1 = keyDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param1.GetValue());
                                ++paramIter;

                                // Value to insert
                                behaviorParameter = method->GetArgument(2);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param2 = valueDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param2.GetValue());
                                ++paramIter;
                            }
                        }

                        AZStd::vector<SlotId> resultSlotIDs;
                        resultSlotIDs.push_back();
                        BehaviorContextMethodHelper::Call(*this, false, method, paramFirst, paramIter, resultSlotIDs);

                        // Push the source container as an output to support chaining
                        PushOutput(*containerDatum, (*outputSlot));
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorInsert::OnInputSignal(const SlotId& slotId)
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
