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

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <AzCore/Component/TickBus.h>

#include <Include/ScriptCanvas/Libraries/Time/Timer.generated.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class Timer 
                : public Node
                , AZ::TickBus::Handler
            {
                ScriptCanvas_Node(Timer,
                    ScriptCanvas_Node::Uuid("{60CF8540-E51A-434D-A32C-461C41D68AF9}")
                    ScriptCanvas_Node::Description("Provides a time value.")
                    ScriptCanvas_Node::Version(2)
                );

            public:

                Timer();

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Start", "Starts the timer."));
                ScriptCanvas_In(ScriptCanvas_In::Name("Stop", "Stops the timer."));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled every frame while the timer is running."));

                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Milliseconds", "The amount of time that has elapsed since the timer started in milliseconds.")
                    ScriptCanvas_Property::Visibility(false)
                    ScriptCanvas_Property::Transient
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Seconds", "The amount of time that has elapsed since the timer started in seconds.")
                    ScriptCanvas_Property::Visibility(false)
                    ScriptCanvas_Property::Transient
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );
                
                // Temps
                float m_seconds;
                float m_milliseconds;
            
            protected:

                AZ::ScriptTimePoint m_start;

                void OnInputSignal(const SlotId& slotId) override;

                void OnDeactivate() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            };
        }
    }
}