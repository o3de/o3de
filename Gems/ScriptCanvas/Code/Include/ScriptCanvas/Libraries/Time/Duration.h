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

#include <Include/ScriptCanvas/Libraries/Time/Duration.generated.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class Duration 
                : public Node
                , AZ::TickBus::Handler
            {
                ScriptCanvas_Node(Duration,
                    ScriptCanvas_Node::Uuid("{D93538FF-3553-4C65-AB81-9089C5270214}")
                    ScriptCanvas_Node::Description("Triggers a signal every frame during the specified duration.")
                    ScriptCanvas_Node::Version(2, DurationNodeVersionConverter)
                );

                static bool DurationNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            public:

                Duration();

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Start", "Starts the countdown"));

                // Outputs
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Out", "Signaled every frame while the duration is active."));
                ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("Done", "Signaled once the duration is complete."));

                // Data
                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Duration", "The time this node will remain active.")
                    ScriptCanvas_Property::Input
                    ScriptCanvas_Property::Min(0.f));


                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Elapsed", "The amount of time that has elapsed since the countdown began.")
                    ScriptCanvas_Property::Visibility(false)
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                // temps
                float m_durationSeconds;
                float m_elapsedTime;

                //! Internal counter to track time elapsed
                float m_currentTime;

                void OnInputSignal(const SlotId&) override;
            
            protected:

                void OnDeactivate() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            };
        }
    }
}