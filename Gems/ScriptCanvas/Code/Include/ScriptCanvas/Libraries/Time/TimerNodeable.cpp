/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TimerNodeable.h"

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            void TimerNodeable::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint time)
            {
                AZ_PROFILE_FUNCTION(ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
                double milliseconds = time.GetMilliseconds() - m_start.GetMilliseconds();
                double seconds = time.GetSeconds() - m_start.GetSeconds();
                CallOnTick(milliseconds, seconds);
            }

            void TimerNodeable::Start()
            {
                AZ::TickBus::Handler::BusConnect();
                m_start = AZ::ScriptTimePoint();
            }

            void TimerNodeable::Stop()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }
}
