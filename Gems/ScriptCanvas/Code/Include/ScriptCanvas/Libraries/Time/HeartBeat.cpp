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

            void HeartBeat::OnInputSignal(const SlotId& slotId)
            {
                if (slotId == HeartBeatProperty::GetStartSlotId(this))
                {
                    StartTimer();
                }
                else if (slotId == HeartBeatProperty::GetStopSlotId(this))
                {
                    StopTimer();
                }
            }

            void HeartBeat::OnTimeElapsed()
            {
                SignalOutput(HeartBeatProperty::GetPulseSlotId(this));
            }
        }
    }
}
