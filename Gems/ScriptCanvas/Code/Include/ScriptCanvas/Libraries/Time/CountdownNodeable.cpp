/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CountdownNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            CountdownNodeable::~CountdownNodeable()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void CountdownNodeable::InitiateCountdown(bool reset, float countdownSeconds, bool looping, float holdTime)
            {
                if (reset || !AZ::TickBus::Handler::BusIsConnected())
                {
                    // If we're resetting, we need to disconnect.
                    AZ::TickBus::Handler::BusDisconnect();

                    m_countdownSeconds = countdownSeconds;
                    m_looping = looping;
                    m_holdTime = holdTime;

                    m_currentTime = m_countdownSeconds;

                    AZ::TickBus::Handler::BusConnect();
                }
            }

            void CountdownNodeable::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void CountdownNodeable::OnTick(float deltaTime, AZ::ScriptTimePoint time)
            {
                if (m_currentTime <= 0.f)
                {
                    if (m_holding)
                    {
                        m_holding = false;
                        m_currentTime = m_countdownSeconds;
                        m_elapsedTime = 0.f;
                        return;
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

                    ExecutionOut(AZ_CRC("Done", 0x102de0ab), m_elapsedTime);
                }
                else
                {
                    m_currentTime -= static_cast<float>(deltaTime);
                    m_elapsedTime = m_holding ? 0.f : m_countdownSeconds - m_currentTime;
                }
            }

            void CountdownNodeable::Reset(float countdownSeconds, Data::BooleanType looping, float holdTime)
            {
                InitiateCountdown(true, countdownSeconds, looping, holdTime);
            }

            void CountdownNodeable::Start(float countdownSeconds, Data::BooleanType looping, float holdTime)
            {
                InitiateCountdown(false, countdownSeconds, looping, holdTime);
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Time/CountdownNodeable.generated.cpp>
