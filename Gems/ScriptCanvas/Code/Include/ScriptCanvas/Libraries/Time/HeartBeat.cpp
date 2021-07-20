/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScriptCanvas/Libraries/Time/HeartBeat.h>
#include <ScriptCanvas/Libraries/Time/HeartBeatNodeable.h>

#include <ScriptCanvas/Utils/VersionConverters.h>

#include <Include/ScriptCanvas/Libraries/Time/HeartBeatNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //////////////
            // HeartBeat
            //////////////
            void HeartBeat::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                if (auto nodeableNode = azrtti_cast<const Nodes::HeartBeatNodeableNode*>(replacementNode))
                {
                    if (auto nodeable = azrtti_cast<Nodeables::Time::HeartBeatNodeable*>(nodeableNode->GetMutableNodeable()))
                    {
                        nodeable->SetTimeUnits(static_cast<int>(GetTimeUnits()));
                    }
                }

                auto newSlotIds = replacementNode->GetSlotIds(GetBaseTimeSlotName());
                auto oldSlots = GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                if (newSlotIds.size() == 1 && oldSlots.size() == 1 && oldSlots[0]->GetName() == GetBaseTimeSlotName())
                {
                    outSlotIdMap.emplace(oldSlots[0]->GetId(), AZStd::vector<SlotId>{ newSlotIds[0] });
                }
            }

        }
    }
}
