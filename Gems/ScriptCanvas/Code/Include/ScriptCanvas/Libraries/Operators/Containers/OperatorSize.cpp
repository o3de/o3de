/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorSize.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            /////////////////
            // OperatorSize
            /////////////////
            void OperatorSize::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
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

            bool OperatorSize::OperatorSizeVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& classElement)
            {
                // Remove the now unnecessary OperatorBase class from the inheritance chain.
                if (classElement.GetVersion() < 1)
                {
                    AZ::SerializeContext::DataElementNode* operatorBaseClass = classElement.FindSubElement(AZ::Crc32("BaseClass1"));

                    if (operatorBaseClass == nullptr)
                    {
                        return false;
                    }

                    int nodeElementIndex = operatorBaseClass->FindElement(AZ_CRC("BaseClass1", 0xd4925735));

                    if (nodeElementIndex < 0)
                    {
                        return false;
                    }

                    // The DataElementNode is being copied purposefully in this statement to clone the data
                    AZ::SerializeContext::DataElementNode baseNodeElement = operatorBaseClass->GetSubElement(nodeElementIndex);

                    classElement.RemoveElementByName(AZ::Crc32("BaseClass1"));
                    classElement.AddElement(baseNodeElement);                    
                }

                return true;
            }

            void OperatorSize::OnInit()
            {
                // Version Conversion away from Operator Base
                if (HasSlots())
                {
                    const Slot* slot = GetSlot(OperatorSizeProperty::GetSizeSlotId(this));
                    if (slot == nullptr)
                    {
                        ConfigureSlots();
                    }
                }
                ////
            }

            void OperatorSize::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorSizeProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {

                    SlotId sourceSlotId = OperatorSizeProperty::GetSourceSlotId(this);
                    SlotId sizeSlotId = OperatorSizeProperty::GetSizeSlotId(this);

                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        // Get the size of the container
                        auto sizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Size");
                        if (!sizeOutcome)
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of container: %s", sizeOutcome.GetError().c_str());
                            return;
                        }

                        // Index
                        Datum sizeResult = sizeOutcome.TakeValue();

                        PushOutput(sizeResult, *GetSlot(sizeSlotId));                        
                    }
                    else
                    {
                        Datum zero(0);
                        PushOutput(zero, *GetSlot(sizeSlotId));
                    }

                    SignalOutput(OperatorSizeProperty::GetOutSlotId(this));
                }
            }
        }
    }
}
