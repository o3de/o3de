/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TimeDelayNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            void TimeDelayNodeable::Start(Data::NumberType time)
            {
                if (!IsActive())
                {
                    StartTimer(time);
                }
            }

            bool TimeDelayNodeable::AllowInstantResponse() const
            {
                return true;
            }

            void TimeDelayNodeable::OnTimeElapsed()
            {
                CallDone();
                StopTimer();
            }
        }
    }
}
