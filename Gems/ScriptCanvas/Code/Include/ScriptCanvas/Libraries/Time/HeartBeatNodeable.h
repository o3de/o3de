/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Internal/Nodeables/BaseTimer.h>

#include <Include/ScriptCanvas/Libraries/Time/HeartBeatNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class HeartBeatNodeable
                : public Nodeables::Time::BaseTimer
            {
                SCRIPTCANVAS_NODE(HeartBeatNodeable);

            protected:
                void OnTimeElapsed() override;
            };
        }
    }
}
