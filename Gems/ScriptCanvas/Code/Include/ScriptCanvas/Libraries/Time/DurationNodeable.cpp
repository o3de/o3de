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
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
                SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(GetScriptCanvasId(), GetAssetId());

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
                m_duration = duration;
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }
}
