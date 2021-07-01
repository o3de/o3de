/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Timer.h"

#include <ScriptCanvas/Utils/VersionConverters.h>

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
