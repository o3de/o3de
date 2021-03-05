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
#include <AzCore/RTTI/TypeInfo.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

#include <Include/ScriptCanvas/Libraries/Time/Countdown.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class TimeDelay
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {
                ScriptCanvas_Node(TimeDelay,
                    ScriptCanvas_Node::Name("Time Delay")
                    ScriptCanvas_Node::Uuid("{364F5AC9-8351-44B6-A069-03367B21F7AA}")
                    ScriptCanvas_Node::Description("Delays all incoming execution for the specified number of ticks")
                );

            public:
                TimeDelay() = default;

                void OnInputSignal(const SlotId&) override;

                bool AllowInstantResponse() const override;
                void OnTimeElapsed() override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled, execution is delayed at this node for the specified amount of times.")
                    ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Out", "Signaled after waiting for the specified amount of times."));
            };

            class TickDelay
                : public Node
                , AZ::TickBus::Handler
                , AZ::SystemTickBus::Handler
            {
                ScriptCanvas_Node(TickDelay,
                    ScriptCanvas_Node::Name("Tick Delay")
                    ScriptCanvas_Node::Uuid("{399A2608-77E3-41F9-90FA-58A9B6E0E34D}")
                    ScriptCanvas_Node::Description("Delays all incoming execution for the specified number of ticks")
                    ScriptCanvas_Node::Deprecated("Tick Delay has been replaced with the generic Time Delay")
                );
            public:

                TickDelay();

                void OnDeactivate() override;
                void OnInputSignal(const SlotId&) override;

                // SystemTickBus
                void OnSystemTick() override;
                ////

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;
                int GetTickOrder() override;
                ////

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled, execution is delayed at this node for the specified amount of frames.")
                    ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Out", "Signaled after waiting for the specified amount of frames."));

                // Data
                ScriptCanvas_PropertyWithDefaults(int, 1,
                    ScriptCanvas_Property::Name("Ticks", "The amount of ticks that need to occur before exeuction triggers")
                    ScriptCanvas_Property::Input
                    ScriptCanvas_Property::Default
                );

                ScriptCanvas_PropertyWithDefaults(int, static_cast<int>(AZ::TICK_DEFAULT),
                    ScriptCanvas_Property::Name("Tick Order", "Where in the Tick Order this delay should happen")
                    ScriptCanvas_Property::Input
                    ScriptCanvas_Property::Default
                );

            private:
                int m_tickCounter;
                int m_tickOrder;
            };

            class Countdown 
                : public Node
                , AZ::TickBus::Handler
            {
                ScriptCanvas_Node(Countdown,
                    ScriptCanvas_Node::Name("Delay")
                    ScriptCanvas_Node::Uuid("{FAEADF5A-F7D9-415A-A3E8-F534BD379B9A}")
                    ScriptCanvas_Node::Description("Counts down time from a specified value.")
                    ScriptCanvas_Node::Version(2, CountdownNodeVersionConverter)
                );

                static bool CountdownNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            public:

                Countdown();

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled, execution is delayed at this node according to the specified properties.")
                                ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                ScriptCanvas_In(ScriptCanvas_In::Name("Reset", "Resets the delay.")
                                ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Out", "Signaled when the delay reaches zero."));

                // Data
                ScriptCanvas_PropertyWithDefaults(float, 1.0f,
                    ScriptCanvas_Property::Name("Time", "Amount of time to delay, in seconds") 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(bool,
                    ScriptCanvas_Property::Name("Loop", "If true, the delay will restart after triggering the Out slot") ScriptCanvas_Property::ChangeNotify(AZ::Edit::PropertyRefreshLevels::EntireTree) 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Hold", "Amount of time to wait before restarting, in seconds") ScriptCanvas_Property::Visibility(&Countdown::ShowHoldTime) 
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(float, 
                    ScriptCanvas_Property::Name("Elapsed", "The amount of time that has elapsed since the delay began.") ScriptCanvas_Property::Visibility(false) 
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                // temps
                float m_countdownSeconds;
                bool m_looping;
                float m_holdTime;
                float m_elapsedTime;

                //! Internal, used to determine if the node is holding before looping
                bool m_holding;

                //! Internal counter to track time elapsed
                float m_currentTime;

                void OnInputSignal(const SlotId&) override;

                bool ShowHoldTime() const
                {
                    // TODO: This only works on the property grid. If a true value is connected to the "SetLoop" slot,
                    // We need to show the "Hold Before Loop" slot, otherwise we need to hide it.
                    return m_looping;
                }

            protected:

                void OnDeactivate() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            };
        }
    }
}
