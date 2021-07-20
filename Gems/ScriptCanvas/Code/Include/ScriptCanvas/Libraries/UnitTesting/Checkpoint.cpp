/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Checkpoint.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void Checkpoint::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
                const auto report = FindDatum(GetSlotId("Report"))->GetAs<Data::StringType>();

                ScriptCanvas::UnitTesting::Bus::Event(GetOwningScriptCanvasId(), &ScriptCanvas::UnitTesting::BusTraits::Checkpoint, *report);
                
                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
