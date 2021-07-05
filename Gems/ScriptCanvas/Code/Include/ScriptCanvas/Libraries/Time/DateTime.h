/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <AzCore/Component/TickBus.h>

#include <Include/ScriptCanvas/Libraries/Time/DateTime.generated.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            class DateTime
                : public Node
                , AZ::TickBus::Handler
                , AZ::SystemTickBus::Handler
            {
                ScriptCanvas_Node(DateTime,
                    ScriptCanvas_Node::Name("Date Time")
                    ScriptCanvas_Node::Uuid("")
                    ScriptCanvas_Node::Description("")
                );
            public:
                DateTime();

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

        }
    }
}
