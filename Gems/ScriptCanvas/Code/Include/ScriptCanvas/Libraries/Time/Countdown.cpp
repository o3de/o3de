/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Countdown.h"
#include <ScriptCanvas/Libraries/Time/TimeDelayNodeable.h>

#include <ScriptCanvas/Core/Contracts.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //////////////
            // TimeDelay
            //////////////
            void TimeDelay::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                if (auto nodeableNode = azrtti_cast<const Nodes::TimeDelayNodeableNode*>(replacementNode))
                {
                    if (auto nodeable = azrtti_cast<Nodeables::Time::TimeDelayNodeable*>(nodeableNode->GetMutableNodeable()))
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

            bool TimeDelay::AllowInstantResponse() const
            {
                return true;
            }

            //////////////
            // TickDelay
            //////////////
            void TickDelay::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                if (auto nodeableNode = azrtti_cast<const Nodes::TimeDelayNodeableNode*>(replacementNode))
                {
                    if (auto nodeable = azrtti_cast<Nodeables::Time::TimeDelayNodeable*>(nodeableNode->GetMutableNodeable()))
                    {
                        nodeable->SetTimeUnits(static_cast<int>(Nodes::Internal::BaseTimerNode::TimeUnits::Ticks));
                    }
                }

                auto newSlotId = replacementNode->GetSlotId("Delay");
                outSlotIdMap.emplace(TickDelayProperty::GetTicksSlotId(this), AZStd::vector<SlotId>{ newSlotId });
                outSlotIdMap.emplace(TickDelayProperty::GetTickOrderSlotId(this), AZStd::vector<SlotId>());
            }

            TickDelay::TickDelay()
                : Node()
                , m_tickCounter(0)
                , m_tickOrder(AZ::TICK_DEFAULT)
            {
            }

            //////////////
            // CountDown
            //////////////

            Countdown::Countdown()
                : Node()
                , m_countdownSeconds(0.f)
                , m_looping(false)
                , m_holdTime(0.f)
                , m_holding(false)
                , m_currentTime(0.)
            {}

            bool Countdown::IsOutOfDate(const VersionData& graphVersion) const
            {
                AZ_UNUSED(graphVersion);
                return !CountdownProperty::GetCancelSlotId(this).IsValid();
            }

            UpdateResult Countdown::OnUpdateNode()
            {
                // Add in the missing cancel slot
                ExecutionSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = "Cancel";
                slotConfiguration.m_toolTip = "Cancels the current delay.";

                slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);

                slotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                {
                    // Contract: DisallowReentrantExecutionContract
                    { []() { return aznew DisallowReentrantExecutionContract; } }
                };

                AddSlot(slotConfiguration);

                return UpdateResult::DirtyGraph;
            }
        }
    }
}
