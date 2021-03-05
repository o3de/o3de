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
