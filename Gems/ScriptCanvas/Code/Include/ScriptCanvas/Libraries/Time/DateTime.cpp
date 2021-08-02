/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DateTime.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            ///////////////
            // DateTime
            ///////////////

            DateTime::DateTime()
                : Node()
                , m_tickCounter(0)
                , m_tickOrder(AZ::TICK_DEFAULT)
            {
            }

            void DateTime::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
                AZ::SystemTickBus::Handler::BusDisconnect();
            }

            void DateTime::OnInputSignal(const SlotId&)
            {
                m_tickCounter = DateTimeProperty::GetTicks(this);

                if (m_tickCounter >= 0)
                {
                    if (!AZ::SystemTickBus::Handler::BusIsConnected())
                    {
                        AZ::SystemTickBus::Handler::BusConnect();
                    }
                }
            }

            void DateTime::OnSystemTick()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();

                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }

                m_tickOrder = DateTimeProperty::GetTickOrder(this);
                AZ::TickBus::Handler::BusConnect();
            }

            void DateTime::OnTick(float deltaTime, AZ::ScriptTimePoint timePoint)
            {
                --m_tickCounter;

                if (m_tickCounter <= 0)
                {
                    const SlotId outSlot = DateTimeProperty::GetOutSlotId(this);

                    SignalOutput(outSlot);
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            int DateTime::GetTickOrder()
            {
                return m_tickOrder;
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Time/DateTime.generated.cpp>
