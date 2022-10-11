/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DurationNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            DurationNodeable::~DurationNodeable()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void DurationNodeable::OnDeactivate()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void DurationNodeable::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                AZ_PROFILE_FUNCTION(ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE

                if (m_elapsedTime <= m_duration)
                {
                    float elapsedTime = m_elapsedTime;
                    m_elapsedTime += static_cast<float>(deltaTime);
                    CallOnTick(elapsedTime);
                }
                else
                {
                    AZ::TickBus::Handler::BusDisconnect();
                    CallDone();
                }
            }

            void DurationNodeable::Start(Data::NumberType duration)
            {
                m_elapsedTime = 0.0f;
                m_duration = static_cast<float>(duration);
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }
}
