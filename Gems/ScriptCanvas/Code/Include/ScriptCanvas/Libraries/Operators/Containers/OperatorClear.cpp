/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorClear.h"

#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Core.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorClear::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
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

            bool OperatorClear::OperatorClearVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() < Version::RemoveOperatorBase)
                {
                    // Remove OperatorBase
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }
                }

                return true;
            }

            void OperatorClear::OnInit()
            {
                // Verisoning for bad initial setup of output container slot
                Slot* slot = OperatorClearProperty::GetContainerSlot(this);

                if (slot && slot->GetDynamicGroup() == AZ::Crc32())
                {
                    SetDynamicGroup(slot->GetId(), AZ::Crc32("ContainerGroup"));
                }
                ////
            }

            void OperatorClear::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);

                if (slotId != inSlotId)
                {
                    return;
                }

                SlotId sourceSlotId = OperatorClearProperty::GetSourceSlotId(this);

                if (const Datum* containerDatum = FindDatum(sourceSlotId))
                {
                    if (Datum::IsValidDatum(containerDatum))
                    {
                        AZ::Outcome<Datum, AZStd::string> clearOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Clear");
                        if (!clearOutcome.IsSuccess())
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Clear on container: %s", clearOutcome.GetError().c_str());
                            return;
                        }

                        // Push the source container as an output to support chaining
                        PushOutput(*containerDatum, *OperatorClearProperty::GetContainerSlot(this));
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
