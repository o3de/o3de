/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
