/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorEmpty.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //////////////////
            // OperatorEmpty
            //////////////////
            void OperatorEmpty::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
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

            bool OperatorEmpty::OperatorEmptyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < 1)
                {
                    // Remove OperatorBase
                    if (!SerializationUtils::RemoveBaseClass(context, classElement))
                    {
                        return false;
                    }
                }

                return true;
            }

            void OperatorEmpty::OnInit()
            {
                // Version Conversion away from Operator Base
                if (HasSlots())
                {
                    const Slot* slot = GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this));
                    if (slot == nullptr)
                    {
                        ConfigureSlots();
                    }
                }
                ////
            }

            void OperatorEmpty::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorEmptyProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    SlotId sourceSlotId = OperatorEmptyProperty::GetSourceSlotId(this);

                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        // Is the container empty?
                        auto emptyOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Empty");
                        if (!emptyOutcome)
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Empty on container: %s", emptyOutcome.GetError().c_str());
                            return;
                        }

                        Datum emptyResult = emptyOutcome.TakeValue();
                        bool isEmpty = *emptyResult.GetAs<bool>();

                        PushOutput(Datum(isEmpty), *GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this)));

                        if (isEmpty)
                        {
                            SignalOutput(OperatorEmptyProperty::GetTrueSlotId(this));
                        }
                        else
                        {
                            SignalOutput(OperatorEmptyProperty::GetFalseSlotId(this));
                        }
                    }
                    else
                    {
                        PushOutput(Datum(true), *GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this)));
                        SignalOutput(OperatorEmptyProperty::GetFalseSlotId(this));
                    }

                    SignalOutput(OperatorEmptyProperty::GetOutSlotId(this));
                }
            }
        }
    }
}
