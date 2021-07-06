/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Multiplexer.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Multiplexer::Multiplexer()
            {}

            void Multiplexer::OnInputSignal(const SlotId& slotId)
            {
                auto slotIndex = FindSlotIndex(slotId);
                if (!slotIndex)
                {
                    AZ_Warning("Script Canvas", false, "Could not find slot with Id %s", slotId.ToString().c_str());
                    return;
                }

                // These are generated from CodeGen, they essentially
                // check if there's anything connected on the slot and
                // they assign the local member with the value, otherwise
                // they use the default property value.
                const AZ::s64 selectedIndex = MultiplexerProperty::GetIndex(this);

                if (selectedIndex == slotIndex)
                {
                    const SlotId outSlotId = MultiplexerProperty::GetOutSlotId(this);
                    SignalOutput(outSlotId);
                }
            }
        }
    }
}
