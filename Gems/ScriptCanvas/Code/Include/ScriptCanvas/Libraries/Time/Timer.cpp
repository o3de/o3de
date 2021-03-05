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

#include "Timer.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            Timer::Timer()
                : Node()
                , m_seconds(0.f)
                , m_milliseconds(0.f)
            {}

            void Timer::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint time)
            {
                const SlotId millisecondsSlot = TimerProperty::GetMillisecondsSlotId(this);

                double milliseconds = time.GetMilliseconds() - m_start.GetMilliseconds();
                Datum objms(Data::Type::Number(), Datum::eOriginality::Copy);
                objms.Set(milliseconds);
                if (auto* slot = GetSlot(millisecondsSlot))
                {
                    PushOutput(objms, *slot);
                }

                const SlotId secondsSlot = TimerProperty::GetSecondsSlotId(this);

                double seconds = time.GetSeconds() - m_start.GetSeconds();
                Datum objs(Data::Type::Number(), Datum::eOriginality::Copy);
                objs.Set(seconds);
                if (auto* slot = GetSlot(secondsSlot))
                {
                    PushOutput(objs, *slot);
                }

                const SlotId outSlot = TimerProperty::GetOutSlotId(this);
                SignalOutput(outSlot);

            }

            void Timer::OnInputSignal(const SlotId& slotId)
            {
                const SlotId startSlot = TimerProperty::GetStartSlotId(this);
                const SlotId stopSlot = TimerProperty::GetStopSlotId(this);

                if (slotId == startSlot)
                {
                    AZ::TickBus::Handler::BusConnect();
                    m_start = AZ::ScriptTimePoint();
                }
                else if (slotId == stopSlot)
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            void Timer::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }
}
