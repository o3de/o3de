/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RepeaterNodeable.h"
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Core
        {
            void RepeaterNodeable::Start(double repetitions, Data::NumberType time)
            {
                m_repetionCount = aznumeric_cast<int>(repetitions);
                if (m_repetionCount > 0)
                {
                    StartTimer(time);
                }
            }

            void RepeaterNodeable::OnTimeElapsed()
            {
                m_repetionCount--;
                CallAction();

                if (m_repetionCount == 0)
                {
                    StopTimer();
                    CallComplete();
                }
            }
        }
    }
}
