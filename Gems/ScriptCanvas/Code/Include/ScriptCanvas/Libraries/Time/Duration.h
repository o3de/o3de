/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <AzCore/Component/TickBus.h>

#include <AzCore/RTTI/TypeInfo.h>

#include <Include/ScriptCanvas/Libraries/Time/Duration.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //! Deprecated: see DurationNodeableNode
            class Duration
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Duration);

                Duration();

            protected:

                float m_durationSeconds;
                float m_elapsedTime;

                //! Internal counter to track time elapsed
                float m_currentTime;
            };
        }
    }
}
