/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Internal/Nodeables/BaseTimer.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Include/ScriptCanvas/Libraries/Time/TimeDelayNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class TimeDelayNodeable
                : public Nodeables::Time::BaseTimer
            {
                SCRIPTCANVAS_NODE(TimeDelayNodeable)

            protected:
                bool AllowInstantResponse() const override;
                void OnTimeElapsed() override;

            };
        }
    }
}
