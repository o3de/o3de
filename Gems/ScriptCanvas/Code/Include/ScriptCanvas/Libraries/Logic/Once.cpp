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

#include <Libraries/Logic/Once.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Once::Once()
                : Node()
                , m_resetStatus(true)
            {}

            void Once::OnInputSignal(const SlotId& slotId)
            {
                SlotId resetSlot = OnceProperty::GetResetSlotId(this);

                if (slotId == resetSlot)
                {
                    m_resetStatus = true;
                }
                else if (m_resetStatus)
                {
                    m_resetStatus = false;
                    SignalOutput(OnceProperty::GetOutSlotId(this));
                }
            }
        }
    }
}
