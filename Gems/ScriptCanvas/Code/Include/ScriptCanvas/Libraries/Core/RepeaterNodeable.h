/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Internal/Nodeables/BaseTimer.h>

#include <Include/ScriptCanvas/Libraries/Core/RepeaterNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Core
        {
            class RepeaterNodeable
                : public Nodeables::Time::BaseTimer
            {
                SCRIPTCANVAS_NODE(RepeaterNodeable)

            protected:

                void OnTimeElapsed() override;
                bool AllowInstantResponse() const override { return true; }

            private:

                int m_repetionCount = 0;
            };
        }
    }
}
