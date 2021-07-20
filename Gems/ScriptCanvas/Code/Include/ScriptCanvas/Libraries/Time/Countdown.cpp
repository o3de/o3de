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

            void TimeDelay::OnInputSignal(const SlotId& slotId)
            {
                if (slotId == TimeDelayProperty::GetInSlotId(this))
                {
                    if (!IsActive())
                    {
                        StartTimer();
                    }
                }
            }

            bool TimeDelay::AllowInstantResponse() const
            {
                return true;
            }

            void TimeDelay::OnTimeElapsed()
            {
                StopTimer();
                SignalOutput(TimeDelayProperty::GetOutSlotId(this));
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

            void TickDelay::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
                AZ::SystemTickBus::Handler::BusDisconnect();
            }

            void TickDelay::OnInputSignal(const SlotId&)
            {
                m_tickCounter = TickDelayProperty::GetTicks(this);

                if (m_tickCounter >= 0)
                {
                    if (!AZ::SystemTickBus::Handler::BusIsConnected())
                    {
                        AZ::SystemTickBus::Handler::BusConnect();
                    }
                }
            }

            void TickDelay::OnSystemTick()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();

                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }

                m_tickOrder = TickDelayProperty::GetTickOrder(this);
                AZ::TickBus::Handler::BusConnect();
            }

            void TickDelay::OnTick(float, AZ::ScriptTimePoint)
            {
                --m_tickCounter;

                if (m_tickCounter <= 0)
                {
                    const SlotId outSlot = TickDelayProperty::GetOutSlotId(this);

                    SignalOutput(outSlot);
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            int TickDelay::GetTickOrder()
            {
                return m_tickOrder;
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

            void Countdown::OnInputSignal(const SlotId& slot)
            {
                SlotId inSlotId = CountdownProperty::GetInSlotId(this);
                SlotId resetSlotId = CountdownProperty::GetResetSlotId(this);
                SlotId cancelSlotId = CountdownProperty::GetCancelSlotId(this);

                if (slot == resetSlotId || (slot == inSlotId && !AZ::TickBus::Handler::BusIsConnected()))
                {
                    // If we're resetting, we need to disconnect.
                    AZ::TickBus::Handler::BusDisconnect();

                    m_countdownSeconds = CountdownProperty::GetTime(this);
                    m_looping = CountdownProperty::GetLoop(this);
                    m_holdTime = CountdownProperty::GetHold(this);
                    
                    m_currentTime = m_countdownSeconds;

                    AZ::TickBus::Handler::BusConnect();
                }
                else if (slot == cancelSlotId)
                {
                    m_holding = false;
                    m_currentTime = 0.f;

                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            bool Countdown::IsOutOfDate(const VersionData& graphVersion) const
            {
                AZ_UNUSED(graphVersion);
                return !CountdownProperty::GetCancelSlotId(this).IsValid();
            }

            void Countdown::OnTick(float deltaTime, AZ::ScriptTimePoint)
            {
                m_currentTime -= static_cast<float>(deltaTime);

                if (m_currentTime <= 0.f)
                {
                    if (m_holding)
                    {
                        m_holding = false;
                        m_currentTime = m_countdownSeconds;
                        return;
                    }
                    else
                    {
                        const SlotId outSlot = CountdownProperty::GetOutSlotId(this);
                        
                        if (Slot* elapsedSlot = CountdownProperty::GetElapsedSlot(this))
                        {
                            float elapsedTime = m_countdownSeconds - m_currentTime;

                            Datum o(Data::Type::Number(), Datum::eOriginality::Copy);
                            o.Set(elapsedTime);

                            PushOutput(o, *elapsedSlot);
                        }

                        SignalOutput(outSlot);
                    }

                    if (!m_looping)
                    {
                        AZ::TickBus::Handler::BusDisconnect();
                    }
                    else
                    {
                        m_holding = m_holdTime > 0.f;
                        m_currentTime = m_holding ? m_holdTime : m_countdownSeconds;
                    }
                }
            }

            void Countdown::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
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
