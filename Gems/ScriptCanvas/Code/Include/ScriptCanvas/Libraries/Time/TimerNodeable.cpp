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

#include "TimerNodeable.h"

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            void TimerNodeable::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint time)
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(GetScriptCanvasId(), GetAssetId());
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
