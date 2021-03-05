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

#include "Countdown.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //////////////
            // TimeDelay
            //////////////

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
                SignalOutput(TimeDelayProperty::GetOutSlotId(this));
                StopTimer();
            }

            //////////////
            // TickDelay
            //////////////

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

            void TickDelay::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
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

            bool Countdown::CountdownNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                // Fixed issue with out pins not being correctly marked as latent.
                if (classElement.GetVersion() < 2)
                {
                    const char* slotsKey = "Slots";
                    AZ::Crc32 slotsId = AZ::Crc32(slotsKey);

                    AZStd::list< ScriptCanvas::Slot > nodeSlots;

                    AZ::SerializeContext::DataElementNode* baseClassElement = classElement.FindSubElement(AZ::Crc32("BaseClass1"));
                    AZ::SerializeContext::DataElementNode* dataNode = baseClassElement->FindSubElement(slotsId);

                    if (dataNode && dataNode->GetData(nodeSlots))
                    {
                        baseClassElement->RemoveElementByName(slotsId);

                        for (ScriptCanvas::Slot& slot : nodeSlots)
                        {
                            if (slot.GetName() == "Out")
                            {
                                slot.ConvertToLatentExecutionOut();
                            }
                        }

                        baseClassElement->AddElementWithData(context, slotsKey, nodeSlots);
                    }
                }

                return true;
            }

            //////////////
            // CountDown
            //////////////

            Countdown::Countdown()
                : Node()
                , m_countdownSeconds(0.f)
                , m_looping(false)
                , m_holdTime(0.f)
                , m_elapsedTime(0.f)
                , m_holding(false)
                , m_currentTime(0.)
            {}            

            void Countdown::OnInputSignal(const SlotId& slot)
            {
                const SlotId& inSlotId = CountdownProperty::GetInSlotId(this);
                const SlotId& resetSlotId = CountdownProperty::GetResetSlotId(this);

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
            }

            void Countdown::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                const SlotId outSlot = CountdownProperty::GetOutSlotId(this);
                const SlotId loopingSlot = CountdownProperty::GetLoopSlotId(this);

                if (m_currentTime <= 0.f)
                {
                    if (m_holding)
                    {
                        m_holding = false;
                        m_currentTime = m_countdownSeconds;
                        m_elapsedTime = 0.f;
                        return;
                    }
                    else
                    {
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
                else
                {
                    m_currentTime -= static_cast<float>(deltaTime);
                    m_elapsedTime = m_holding ? 0.f : m_countdownSeconds - m_currentTime;

                    const SlotId elapsedSlot = CountdownProperty::GetElapsedSlotId(this);

                    Datum o(Data::Type::Number(), Datum::eOriginality::Copy);
                    o.Set(m_elapsedTime);
                    if (auto* slot = GetSlot(elapsedSlot))
                    {
                        PushOutput(o, *slot);
                    }

                }
            }

            void Countdown::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }
}
