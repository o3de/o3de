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
