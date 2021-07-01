/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Duration.h"

#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            Duration::Duration()
                : Node()
                , m_durationSeconds(0.f)
                , m_elapsedTime(0.f)
                , m_currentTime(0.)
            {}

            void Duration::OnInputSignal(const SlotId&)
            {
                m_durationSeconds = DurationProperty::GetDuration(this);
                m_elapsedTime = 0.f;

                m_currentTime = m_durationSeconds;

                AZ::TickBus::Handler::BusConnect();
            }

            void Duration::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                const SlotId outSlot = DurationProperty::GetOutSlotId(this);
                const SlotId doneSlot = DurationProperty::GetDoneSlotId(this);
                const SlotId elapsedSlot = DurationProperty::GetElapsedSlotId(this);

                if (m_currentTime > 0.f)
                {
                    Datum o(Data::Type::Number(), Datum::eOriginality::Copy);
                    o.Set(m_elapsedTime);
                    if (auto* slot = GetSlot(elapsedSlot))
                    {
                        PushOutput(o, *slot);
                    }

                    SignalOutput(outSlot);

                    m_currentTime -= static_cast<float>(deltaTime);
                    m_elapsedTime += static_cast<float>(deltaTime);
                }
                else
                {
                    SignalOutput(doneSlot);
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            void Duration::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }
}
