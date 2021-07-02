/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Format.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            void Format::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Format::OnInputSignal");

                Datum output = Datum(ProcessFormat());
                
                const SlotId outputTextSlotId = FormatProperty::GetStringSlotId(this);
                if (auto* slot = GetSlot(outputTextSlotId))
                {
                    PushOutput(output, *slot);
                }

                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
