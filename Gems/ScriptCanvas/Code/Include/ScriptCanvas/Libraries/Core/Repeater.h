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

#include <Include/ScriptCanvas/Libraries/Core/Repeater.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class Repeater
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {
                ScriptCanvas_Node(Repeater,
                    ScriptCanvas_Node::Name("Repeater", "Repeats the output signal the given number of times using the specified delay to space the signals out")
                    ScriptCanvas_Node::Uuid("{0A38EDCA-0571-48F0-9199-F6168C1EAAF0}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Placeholder.png")
                    ScriptCanvas_Node::Category("Utilities")
                    ScriptCanvas_Node::Version(2, VersionConverter)
                );

            public:

                static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));
                
                ScriptCanvas_PropertyWithDefaults(double, 0.0,
                    ScriptCanvas_Property::Name("Repetitions", "How many times to trigger the action pin.")
                    ScriptCanvas_Property::Input
                    ScriptCanvas_Property::Default
                );
                ////
                
                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Complete", "Signaled upon node exit"));
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Action", "The signal that will be repeated"));
                ////

                void OnInit() override;

                void OnInputSignal(const SlotId& slotId) override;
                void OnTimeElapsed();

                const char* GetTimeSlotFormat() const override { return "Delay (%s)"; }
                
            protected:

                bool AllowInstantResponse() const override { return true; }

                int                 m_repetionCount;
            };
        }
    }
}