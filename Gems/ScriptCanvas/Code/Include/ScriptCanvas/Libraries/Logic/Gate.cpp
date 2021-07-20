/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Logic/Gate.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Gate::Gate()
                : Node()
                , m_condition(false)
            {}

            AZ::Outcome<DependencyReport, void> Gate::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }

            void Gate::OnInputSignal(const SlotId&)
            {
                SlotId trueSlot = GateProperty::GetTrueSlotId(this);
                SlotId falseSlot = GateProperty::GetFalseSlotId(this);

                m_condition = GateProperty::GetCondition(this);
                if (m_condition)
                {
                    SignalOutput(trueSlot);
                }
                else
                {
                    SignalOutput(falseSlot);
                }
            }
        }
    }
}
