/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "HeartBeatNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            void HeartBeatNodeable::Start(Data::NumberType time)
            {
                StartTimer(time);
            }

            void HeartBeatNodeable::Stop()
            {
                StopTimer();
            }

            void HeartBeatNodeable::OnTimeElapsed()
            {
                CallPulse();
            }
        }
    }
}
