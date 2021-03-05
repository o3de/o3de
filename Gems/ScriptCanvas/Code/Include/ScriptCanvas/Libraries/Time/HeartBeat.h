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

#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

#include <Include/ScriptCanvas/Libraries/Time/HeartBeat.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class HeartBeat
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {
                ScriptCanvas_Node(HeartBeat,
                    ScriptCanvas_Node::Name("HeartBeat", "While active, will signal the output at the given interval.")
                    ScriptCanvas_Node::Uuid("{BA107060-249D-4818-9CEC-7573718273FC}")
                    ScriptCanvas_Node::Category("Timing")
                    ScriptCanvas_Node::Version(0)
                );

            public:

                void OnInputSignal(const SlotId&) override;

            protected:

                void OnTimeElapsed() override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Start", ""));

                ScriptCanvas_In(ScriptCanvas_In::Name("Stop", ""));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Pulse", ""));
            };
        }
    }
}
