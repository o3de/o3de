/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Include/ScriptCanvas/Libraries/Time/CountdownNodeable.generated.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Time
        {
            class CountdownNodeable 
                : public ScriptCanvas::Nodeable
                , public AZ::TickBus::Handler
            {
                NodeDefinition(CountdownNodeable, "Delay", "Counts down time from a specified value.",
                    NodeTags::Category("Timing"),
                    NodeTags::Version(0)
                );

            public:
                virtual ~CountdownNodeable();

                InputMethod("Start", "When signaled, execution is delayed at this node according to the specified properties.",
                    SlotTags::Contracts({ DisallowReentrantExecutionContract }))
                    void Start(float countdownSeconds, Data::BooleanType looping, float holdTime);

                DataInput(float, "Start: Time", 0.0f, "", SlotTags::DisplayGroup("Start"));
                DataInput(Data::BooleanType, "Start: Loop", false, "", SlotTags::DisplayGroup("Start"));
                DataInput(float, "Start: Hold", 0.0f, "", SlotTags::DisplayGroup("Start"));

                InputMethod("Reset", "When signaled, execution is delayed at this node according to the specified properties.",
                    SlotTags::Contracts({ DisallowReentrantExecutionContract }))
                    void Reset(float countdownSeconds, Data::BooleanType looping, float holdTime);

                DataInput(float, "Reset: Time", 0.0f, "", SlotTags::DisplayGroup("Reset"));
                DataInput(Data::BooleanType, "Reset: Loop", false, "", SlotTags::DisplayGroup("Reset"));
                DataInput(float, "Reset: Hold", 0.0f, "", SlotTags::DisplayGroup("Reset"));

                ExecutionLatentOutput("Done", "Signaled when the delay reaches zero.");
                DataOutput(float, "Elapsed", 0.0f, "The amount of time that has elapsed since the delay began.",
                    SlotTags::DisplayGroup("Done"));

            protected:
                void OnDeactivate() override;

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            private:
                void InitiateCountdown(bool reset, float countdownSeconds, bool looping, float holdTime);

                float m_countdownSeconds = 0.0f;
                bool m_looping = false;
                float m_holdTime = 0.0f;
                float m_elapsedTime = 0.0f;
                bool m_holding = false;
                float m_currentTime = 0.0f;
            };
        }
    }
}
