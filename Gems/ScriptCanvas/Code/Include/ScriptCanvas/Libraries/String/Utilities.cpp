/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Utilities.h"

#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            const char* Split::k_defaultDelimiter = " ";
            const char* Join::k_defaultSeparator = " ";

            void ReplaceStringUtilityNodeOutputSlot(const Node* oldNode, const Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap)
            {
                auto newDataOutSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                auto oldDataOutSlots = oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                if (newDataOutSlots.size() == oldDataOutSlots.size())
                {
                    for (size_t index = 0; index < newDataOutSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataOutSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataOutSlots[index]->GetId() });
                    }
                }
            }

            void Split::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                ReplaceStringUtilityNodeOutputSlot(this, replacementNode, outSlotIdMap);
            }

            void Join::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                ReplaceStringUtilityNodeOutputSlot(this, replacementNode, outSlotIdMap);
            }
        }
    }
}
