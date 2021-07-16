/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Core/Repeater.h>
#include <ScriptCanvas/Libraries/Core/RepeaterNodeable.h>

#include <Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            /////////////
            // Repeater
            /////////////
            void Repeater::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                if (auto nodeableNode = azrtti_cast<const Nodes::RepeaterNodeableNode*>(replacementNode))
                {
                    if (auto nodeable = azrtti_cast<Nodeables::Core::RepeaterNodeable*>(nodeableNode->GetMutableNodeable()))
                    {
                        nodeable->SetTimeUnits(static_cast<int>(GetTimeUnits()));
                    }
                }

                auto newSlotIds = replacementNode->GetSlotIds(GetBaseTimeSlotName());
                auto oldSlots = GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                if (newSlotIds.size() == 1 && oldSlots.size() == 2)
                {
                    for (auto& oldSlot : oldSlots)
                    {
                        if (oldSlot->GetName() == GetBaseTimeSlotName())
                        {
                            outSlotIdMap.emplace(oldSlot->GetId(), AZStd::vector<SlotId>{ newSlotIds[0] });
                            break;
                        }
                    }
                }
            }

            void Repeater::OnInit()
            {
                BaseTimerNode::OnInit();

                AZStd::string slotName = GetTimeSlotName();

                Slot* slot = GetSlotByName(slotName);

                if (slot == nullptr)
                {
                    // Handle older versions and improperly updated names
                    for (auto testUnit : { BaseTimerNode::TimeUnits::Seconds, BaseTimerNode::TimeUnits::Ticks })
                    {
                        AZStd::string legacyName = BaseTimerNode::s_timeUnitNames[static_cast<int>(testUnit)];

                        slot = GetSlotByName(legacyName);

                        if (slot)
                        {
                            slot->Rename(slotName);
                            m_timeSlotId = slot->GetId();
                            break;
                        }
                    }
                }
            }
        }
    }
}
