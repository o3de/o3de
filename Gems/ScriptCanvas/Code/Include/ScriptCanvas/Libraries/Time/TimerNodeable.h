/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Internal/Nodeables/BaseTimer.h>

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Include/ScriptCanvas/Libraries/Time/TimerNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class TimerNodeable
                : public Nodeables::Time::BaseTimer
            {
                SCRIPTCANVAS_NODE(TimerNodeable);
            public:
                AZ_CLASS_ALLOCATOR(TimerNodeable, AZ::SystemAllocator)

            protected:

                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            private:
                AZ::ScriptTimePoint m_start;

            };
        }
    }
}
