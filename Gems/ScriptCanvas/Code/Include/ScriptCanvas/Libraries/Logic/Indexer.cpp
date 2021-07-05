/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Indexer.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            void Indexer::OnInputSignal(const SlotId& slotId)
            {
                auto slotIndex = FindSlotIndex(slotId);
                if (slotIndex < 0)
                {
                    AZ_Warning("Script Canvas", false, "Could not find slot with id %s", slotId.ToString().c_str());
                    return;
                }

                const Datum output(slotIndex);

                const SlotId outSlotId = IndexerProperty::GetOutSlotId(this);
                if (auto outSlot = GetSlot(outSlotId))
                {
                    PushOutput(output, *outSlot);
                }

                SignalOutput(outSlotId);
            }
        }
    }
}
