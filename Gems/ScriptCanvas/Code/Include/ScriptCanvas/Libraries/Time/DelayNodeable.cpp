/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DelayNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            void DelayNodeable::Cancel()
            {
                m_cancelled = true;
                m_holding = false;
                m_currentTime = 0.f;
                AZ::TickBus::Handler::BusDisconnect();
            }

            void DelayNodeable::InitiateCountdown(bool reset, float countdownSeconds, bool looping, float holdTime)
            {
                if (reset || !AZ::TickBus::Handler::BusIsConnected())
                {
                    // If we're resetting, we need to disconnect.
                    AZ::TickBus::Handler::BusDisconnect();

                    m_cancelled = false;
                    m_countdownSeconds = countdownSeconds;
                    m_looping = looping;
                    m_holdTime = holdTime;

                    m_currentTime = m_countdownSeconds;

                    AZ::TickBus::Handler::BusConnect();
                }
            }

            void DelayNodeable::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void DelayNodeable::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                AZ_PROFILE_FUNCTION(ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
                m_currentTime -= static_cast<float>(deltaTime);
                if (m_currentTime <= 0.f)
                {
                    if (!m_looping)
                    {
                        AZ::TickBus::Handler::BusDisconnect();
                    }

                    if (m_holding)
                    {
                        m_holding = false;
                        m_currentTime = m_countdownSeconds;
                        return;
                    }

                    if (!m_cancelled)
                    {
                        float elapsedTime = m_countdownSeconds - m_currentTime;
                        CallDone(elapsedTime);
                    }

                    if (m_looping)
                    {
                        m_holding = m_holdTime > 0.f;
                        m_currentTime = m_holding ? m_holdTime : m_countdownSeconds;
                    }
                }
            }

            void DelayNodeable::Reset(Data::NumberType countdownSeconds, Data::BooleanType looping, Data::NumberType holdTime)
            {
                InitiateCountdown(true, static_cast<float>(countdownSeconds), looping, static_cast<float>(holdTime));
            }

            void DelayNodeable::Start(Data::NumberType countdownSeconds, Data::BooleanType looping, Data::NumberType holdTime)
            {
                InitiateCountdown(false, static_cast<float>(countdownSeconds), looping, static_cast<float>(holdTime));
            }
        }
    }
}
