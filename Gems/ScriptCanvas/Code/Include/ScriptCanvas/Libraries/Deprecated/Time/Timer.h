/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <AzCore/Component/TickBus.h>

#include <Include/ScriptCanvas/Libraries/Deprecated/Time/Timer.generated.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //! Deprecated: See TimerNodeableNode
            class Timer
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(Timer);

                Timer();

                float m_seconds;
                float m_milliseconds;
            };
        }
    }
}
