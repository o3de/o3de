/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            void OperatorErase::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                OperatorBase::CustomizeReplacementNode(replacementNode, outSlotIdMap);

                // Need to map Out slot to Out and Key Not Found slots
                if (m_missedElementNotFound)
                {
                    auto newExecutionOutSlots = replacementNode->GetSlotsByType(CombinedSlotType::ExecutionOut);
                    if (newExecutionOutSlots.size() == 2)
                    {
                        outSlotIdMap.emplace(OperatorBaseProperty::GetOutSlotId(this), AZStd::vector<SlotId>{ newExecutionOutSlots[0]->GetId(), newExecutionOutSlots[1]->GetId() });
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Erase node execution outs expected 2, actual %zu.", newExecutionOutSlots.size());
                    }
                }
            }

            void OperatorErase::OnInit()
            {
                // Version Conversion away from Operator Base
                if (HasSlots())
                {
                    const Slot* slot = GetSlot(OperatorEraseProperty::GetElementNotFoundSlotId(this));
                    if (slot == nullptr)
                    {
                        m_missedElementNotFound = true;
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

        }
    }
}
