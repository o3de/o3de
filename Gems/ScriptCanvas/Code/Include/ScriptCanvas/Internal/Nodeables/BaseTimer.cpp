/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "BaseTimer.h"

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            BaseTimer::BaseTimer()
            {
                m_timeUnitsInterface.SetPropertyReference(&m_timeUnits);

                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Ticks], TimeUnits::Ticks);
                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Milliseconds], TimeUnits::Milliseconds);
                m_timeUnitsInterface.RegisterValueType(s_timeUnitNames[TimeUnits::Seconds], TimeUnits::Seconds);

                m_timeUnitsInterface.RegisterListener(this);
            }

            BaseTimer::~BaseTimer()
            {
                StopTimer();
            }

            void BaseTimer::OnDeactivate()
            {
                StopTimer();
            }

            void BaseTimer::OnSystemTick()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();

                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusConnect();
                }
            }

            void BaseTimer::OnTick(float delta, AZ::ScriptTimePoint)
            {
                AZ_PROFILE_FUNCTION(ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE;

                switch (m_timeUnits)
                {
                case TimeUnits::Ticks:
                    m_timerCounter += 1;
                    break;
                case TimeUnits::Milliseconds:
                case TimeUnits::Seconds:
                    m_timerCounter += delta;
                    break;
                default:
                    break;
                }

                while (m_timerCounter >= m_timerDuration)
                {
                    if (!m_isActive)
                    {
                        break;
                    }

                    m_timerCounter -= m_timerDuration;

                    OnTimeElapsed();
                }
            }

            int BaseTimer::GetTickOrder()
            {
                return m_tickOrder;
            }

            void BaseTimer::SetTimeUnits(int timeUnits)
            {
                m_timeUnits = timeUnits;
            }

            void BaseTimer::StartTimer(Data::NumberType time)
            {
                StopTimer();

                m_isActive = true;

                m_timerDuration = time;
                m_timerCounter = 0;

                // Manage the different unit types
                if (m_timeUnits == TimeUnits::Ticks)
                {
                    // Remove all of the floating points
                    m_timerDuration = aznumeric_cast<float>(aznumeric_cast<int>(m_timerDuration));
                }
                else if (m_timeUnits == TimeUnits::Milliseconds)
                {
                    m_timerDuration /= 1000.0;
                }

                if (AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusDisconnect();
                }

                if (AZ::SystemTickBus::Handler::BusIsConnected())
                {
                    AZ::SystemTickBus::Handler::BusDisconnect();
                }

                if (!AZ::IsClose(m_timerDuration, 0.0, DBL_EPSILON))
                {
                    AZ::SystemTickBus::Handler::BusConnect();
                }
                else if (AllowInstantResponse())
                {
                    while (m_isActive)
                    {
                        OnTimeElapsed();
                    }
                }
            }

            void BaseTimer::StopTimer()
            {
                m_isActive = false;

                m_timerCounter = 0.0;
                m_timerDuration = 0.0;

                AZ::SystemTickBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
            }

            void BaseTimer::OnTimeUnitsChanged(const int&)
            {
                UpdateTimeName();
            }

            void BaseTimer::UpdateTimeName()
            {
                /*Slot* slot = GetSlot(m_timeSlotId);

                if (slot)
                {
                    slot->Rename(GetTimeSlotName());
                }*/
            }

            AZStd::vector<AZStd::pair<int, AZStd::string>> BaseTimer::GetTimeUnitList() const
            {
                AZStd::vector<AZStd::pair<int, AZStd::string>> timeUnits;

                timeUnits.push_back(AZStd::make_pair(TimeUnits::Ticks, s_timeUnitNames[TimeUnits::Ticks]));
                timeUnits.push_back(AZStd::make_pair(TimeUnits::Milliseconds, s_timeUnitNames[TimeUnits::Milliseconds]));
                timeUnits.push_back(AZStd::make_pair(TimeUnits::Seconds, s_timeUnitNames[TimeUnits::Seconds]));

                return timeUnits;
            }
        }
    }
}
