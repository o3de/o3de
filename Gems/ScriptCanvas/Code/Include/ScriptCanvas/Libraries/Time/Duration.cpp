/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Duration.h"

#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            Duration::Duration()
                : Node()
                , m_durationSeconds(0.f)
                , m_elapsedTime(0.f)
                , m_currentTime(0.)
            {}
        }
    }
}
