/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Timer.h"

#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            Timer::Timer()
                : Node()
                , m_seconds(0.f)
                , m_milliseconds(0.f)
            {}
        }
    }
}
