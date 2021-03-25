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

#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Include/ScriptCanvas/Libraries/Time/DurationNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class DurationNodeable 
                : public ScriptCanvas::Nodeable
                , public AZ::TickBus::Handler
            {
                SCRIPTCANVAS_NODE(DurationNodeable);

            public:

                ~DurationNodeable() override;

            protected:
                void OnDeactivate() override;

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            private:
                float m_elapsedTime = 0.0f;
                float m_duration = 0.0f;
            };
        }
    }
}
