/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorDivideByNumber.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>
#include <ScriptCanvas/Data/NumericData.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorDivideByNumber::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                auto newDataInSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                auto oldDataInSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                if (newDataInSlots.size() == oldDataInSlots.size())
                {
                    for (size_t index = 0; index < newDataInSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataInSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataInSlots[index]->GetId() });
                    }
                }

                auto newDataOutSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                auto oldDataOutSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                if (newDataOutSlots.size() == oldDataOutSlots.size())
                {
                    for (size_t index = 0; index < newDataOutSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataOutSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataOutSlots[index]->GetId() });
                    }
                }
            }

            bool OperatorDivideByNumber::OperatorDivideByNumberVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() < Version::RemoveOperatorBase)
                {
                    // Remove ArithmeticOperatorUnary
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }

                    // Remove ArithmeticOperator
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }
                }

                return true;
            }

            void OperatorDivideByNumber::OnInit()
            {
                auto groupedSlots = GetSlotsWithDynamicGroup(GetDynamicGroupId());

                if (groupedSlots.empty())
                {
                    auto inputDataSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataIn());
                    auto outputDataSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataOut());

                    for (const Slot* inputSlot : inputDataSlots)
                    {
                        if (inputSlot->IsDynamicSlot() && inputSlot->GetName().compare("Divisor") != 0)
                        {
                            if (inputSlot->GetDynamicGroup() != GetDynamicGroupId())
                            {
                                SetDynamicGroup(inputSlot->GetId(), GetDynamicGroupId());
                            }
                        }
                    }

                    if (outputDataSlots.size() == 1)
                    {
                        const Slot* resultSlot = outputDataSlots.front();

                        if (resultSlot->GetDynamicGroup() != GetDynamicGroupId())
                        {
                            SetDynamicGroup(resultSlot->GetId(), GetDynamicGroupId());
                        }
                    }

                    groupedSlots = GetSlotsWithDynamicGroup(GetDynamicGroupId());
                }

                for (const Slot* slot : groupedSlots)
                {
                    if (slot->IsInput())
                    {
                        m_operandId = slot->GetId();
                    }
                }
            }

        }
    }
}
